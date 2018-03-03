using System;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;

namespace QMAST
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// 
    /// Created in February 2018, QMAST
    /// </summary>
    public partial class MainWindow
    {

        // UI elements
        Duration defAnimDuration = new Duration(TimeSpan.FromSeconds(0.25)); // Animation duration for header color changes
        Brush brushItemError = new SolidColorBrush(Color.FromArgb(204, 183, 28, 28)); // Red background for error items
        Brush brushItem = new SolidColorBrush(Color.FromArgb(127, 0, 0, 0)); // Normal grey item background
        // Load image resources here
        ImageSource imsXBeeUnplugged = new BitmapImage(new Uri(@"/xbee_unplugged.png", UriKind.Relative));
        ImageSource imsBoatOffline = new BitmapImage(new Uri(@"/lan-pending.png", UriKind.Relative));
        ImageSource imsBoatOnline = new BitmapImage(new Uri(@"/lan-connect.png", UriKind.Relative));
        ImageSource imsBoatError = new BitmapImage(new Uri(@"/alert-circle.png", UriKind.Relative));
        ImageSource imsBoatRC = new BitmapImage(new Uri(@"/gamepad-variant.png", UriKind.Relative));
        ImageSource imsBoatAutopilot = new BitmapImage(new Uri(@"/infinity.png", UriKind.Relative));
        ImageSource imsGPSNoFix = new BitmapImage(new Uri(@"/map-marker-off.png", UriKind.Relative));
        ImageSource imsGPSFix = new BitmapImage(new Uri(@"/map-marker.png", UriKind.Relative));
        // Color animations for the item backgrounds
        ColorAnimation animBlacktoRed;
        ColorAnimation animRedToBlack;
        //Alarm tone for error/offline alerts
        System.Media.SoundPlayer player = new System.Media.SoundPlayer(Properties.Resources.notif);
        bool modeMuted = true;
        bool rPiMuted = true;
        ImageSource imsMuted = new BitmapImage(new Uri(@"/volume-off.png", UriKind.Relative));
        ImageSource imsUnMuted = new BitmapImage(new Uri(@"/volume-high.png", UriKind.Relative));

        // XBee port detection, connection and monitoring 
        SerialPort myPort;
        System.Windows.Threading.DispatcherTimer portTimer = null; // Continuous timer used to detect and connect to the XBee serial port and verify that the XBee is still connected
        string[] detectedPorts; // A list of ports currently connected to the device (one of which could be the XBee)
        int nextPortIndex; // When scanning for a XBee, this indicates the index of the next port to be scanned
        bool portScanning = false; // Indicates if actively scanning for XBee
        int currentState = 0; // Connection status (0 = XBee disconnected, 1 = XBee connected, Boat offline, 3 = Boat online)
        private StringBuilder xbeeInputBuffer = new StringBuilder(); // Holder for incoming data from the XBee

        System.Windows.Threading.DispatcherTimer boatHeartbeatTimer = null; // Timer used to detect when the Mega goes offline/device online pings (heartbeat) stop being recieved
        // The heartbeat timer is set for 2 normal heartbeat periods and reset each time a ping is recieved

        // In-app console
        bool consOutputEnabled = false; // Indicates if text should be written to the console
        private StringBuilder consoleOutputBuffer = new StringBuilder(); // Console output text 

        int lastRudder = -2; // The last transmitted rudder position
        int lastWinch = -2; // The last transmitted winch position
        int boatMode = -1; // Current mode of the Mega, used to ensure UI is only updated if mode has changed
        int rPiState = -1; // Current state of the Raspberry Pi
        bool GPSFixed = false; // If the GPS is connected

        public MainWindow()
        {
            InitializeComponent(); // Build UI

            mwMain.WindowTitleBrush = new SolidColorBrush(Color.FromRgb(183, 28, 28));

            animBlacktoRed = new ColorAnimation();
            animBlacktoRed.Duration = defAnimDuration;
            animBlacktoRed.To = Color.FromArgb(204, 183, 28, 28);
            animBlacktoRed.From = Color.FromArgb(127, 0, 0, 0);

            animRedToBlack = new ColorAnimation();
            animRedToBlack.Duration = defAnimDuration;
            animRedToBlack.From = Color.FromArgb(204, 183, 28, 28);
            animRedToBlack.To = Color.FromArgb(127, 0, 0, 0);

            // Set up and start XBee port monitoring timer
            portTimer = new System.Windows.Threading.DispatcherTimer();
            portTimer.Tick += new EventHandler(portTimer_Tick);
            portTimer.Interval = TimeSpan.FromMilliseconds(10); // Initially scan for devices right away
            portTimer.Start();

            // Set up boat heartbeat timeout timer
            boatHeartbeatTimer = new System.Windows.Threading.DispatcherTimer();
            boatHeartbeatTimer.Interval = TimeSpan.FromMilliseconds(6000);
            boatHeartbeatTimer.Tick += new EventHandler(boatHeartbeatTimer_Tick);
        }

        private void portTimer_Tick(object sender, EventArgs e) // Invoked for each "tick" of the XBee port monitoring timer
        {
            if ((myPort != null && myPort.IsOpen) && !portScanning) { } // Do nothing if an XBee connection is open and active
            else // Scan then loop through each connected serial device, using AT commands to verify that it is actually an XBee
            {
                // When you send "+++" over serial to an XBee, it will enter command mode and respond with "OK"
                // We will be using this attribute to identify a connected serial device as an XBee
                // During each tick of the timer, we will move on to the next serial device, in the meantime, responses are monitored in another function
                portScanning = true;
                portTimer.Interval = TimeSpan.FromMilliseconds(1250); // Speed up the timer tick as much as possible, without missing the response from an XBee

                // Update the state to reflect no XBee connected
                Dispatcher.Invoke((Action)delegate () { setState(0); }); // Must be run on the UI thread
                boatHeartbeatTimer.Stop(); // Stop the timeout timer

                // If no ports were detected last time or if all known ports have been cycled through, get a new list of ports!
                if (detectedPorts == null || detectedPorts.Length == 0 || nextPortIndex >= detectedPorts.Length)
                {
                    // Get a list of serial port names.
                    detectedPorts = SerialPort.GetPortNames();
                    if (detectedPorts.Length != 0)
                    {
                        // If the system has serial devices connected, check the first one
                        nextPortIndex = 0;
                        checkPortsForXBee();
                    }
                }
                else
                {
                    // The previously polled serial device did not respond in time, and is probably not an XBee
                    // Try the next device
                    checkPortsForXBee();
                }
            }
        }

        private void boatHeartbeatTimer_Tick(object sender, EventArgs e)
        {
            // If the timer manages to tick, it means the boat has not responded in two heartbeat intervals
            // The boat is probably off or not in range, update the GUI to reflect that
            Dispatcher.Invoke((Action)delegate () { setState(1);
                //tbCons.Text = tbCons.Text + ("\nTIMEOUT");
            }); // Must be run on the UI thread
            boatHeartbeatTimer.Stop();
        }

        private void checkPortsForXBee()
        {
            // This function will try to to invoke a response from an XBee by polling the next serial device in the list 
            // The logistics of this are described in portTimer_Tick()

            bool portSelected = false;
            // Don't stop until "+++" has been sent to a serial device or we run out of devices
            while (portSelected == false)
            {
                if (detectedPorts == null || detectedPorts.Length == 0 || nextPortIndex >= detectedPorts.Length)
                {
                    // All ports have been cycled through. 
                    // The next tick of the portResponseTimer will refresh detectedPorts with new ports
                    portSelected = true;
                    detectedPorts = null;
                }
                else
                {
                    // Try connecting to the next available serial port
                    if (myPort != null && myPort.IsOpen) myPort.Close(); // Close the previous serial port
                    try
                    {
                        
                        myPort = new SerialPort(detectedPorts[nextPortIndex], 57600);
                        myPort.Open();
                        myPort.DataReceived += new SerialDataReceivedEventHandler(myPort_DataReceived);
                        myPort.Write("+++");
                        xbeeInputBuffer.Clear();
                        portSelected = true; // Only when the port has been opened and written to properly do we exit the loop
                    }
                    catch (System.IO.IOException)
                    {
                        // The current port is in an invalid state
                    }
                    catch (System.UnauthorizedAccessException)
                    {
                        // The current port is busy/access is denied.
                    }
                    catch (System.InvalidOperationException)
                    {
                        // The current port is already open
                    }
                    nextPortIndex++; // Mark the next port index to be polled on the next tick of the timer/call of this method
                }
            }
        }

        // Updates the UI based on the current connection state 
        // (0 = XBee disconnected, 1 = XBee connected, Boat offline, 3 = Boat online)
        private void setState(int state)
        {
            //tbCons.Text = tbCons.Text + "> last state:" + currentState + ", current state:" + state;

            // Update the UI elements associated with losing boat conection through both XBee disconnect and heartbeat timeout
            if ((state == 0 || state == 1) && (currentState == 2)) // If the connection to the boat was just lost...
            {
                // Update the header
                lTitle.Content = "Boat Offline";
                // Turn off/disable servo override
                sServOverride.IsChecked = false;
                sServOverride.IsEnabled = false;
                // Make all the status item backgrounds red
                gMode.Background = brushItemError;
                gRPi.Background = brushItemError;
                gCompass.Background = brushItemError;
                gWind.Background = brushItemError;
                gGPS.Background = brushItemError;
                gTemp.Background = brushItemError;

                boatMode = -1;
            }

            // Update the UI elements only once when first entering the state
            if (state == 0 && currentState != 0) // If the XBee is disconnected...
            {
                //tbCons.Text = tbCons.Text + ("\nOFFLINE");
                // Update the header
                lSubTitle.Content = "XBee Disconnected";
                iState.Source = imsXBeeUnplugged;
                ColorAnimation animation = new ColorAnimation(); // Animate the header color change
                animation.Duration = defAnimDuration;
                animation.To = Color.FromRgb(183, 28, 28);
                // Choose the right color based on what state we are coming from
                if (currentState == 1) { animation.From = Color.FromRgb(230, 81, 0); }
                else { animation.From = Color.FromRgb(1, 87, 155); }
                gState.Background.BeginAnimation(SolidColorBrush.ColorProperty, animation);
                mwMain.WindowTitleBrush = new SolidColorBrush(Color.FromRgb(183, 28, 28));

                // Disable the command input bar
                tbConsInput.IsEnabled = false;
                bConsSend.IsEnabled = false;

                // Disable the console
                sCons.IsEnabled = false;
                sCons.IsChecked = false; // Will also clear the console via an event listener

                currentState = state; // Record the current state
            }
            else if (state == 1 && currentState != 1) // If the boat is offline but the XBee is connected...
            {
                //tbCons.Text = tbCons.Text + ("\nXBEE");
                // Update the header
                lSubTitle.Content = "XBee Connected";
                iState.Source = imsBoatOffline;
                ColorAnimation animation = new ColorAnimation(); // Animate the header color change
                animation.Duration = defAnimDuration;
                animation.To = Color.FromRgb(230, 81, 0);
                // Choose the right color based on what state we are coming from
                if (currentState == 0) { animation.From = Color.FromRgb(183, 28, 28); }
                else { animation.From = Color.FromRgb(1, 87, 155); }
                gState.Background.BeginAnimation(SolidColorBrush.ColorProperty, animation);
                mwMain.WindowTitleBrush = new SolidColorBrush(Color.FromRgb(230, 81, 0));

                // Enable the command input bar
                tbConsInput.IsEnabled = true;

                // Enable the console
                bConsSend.IsEnabled = true;
                sCons.IsEnabled = true;

                currentState = state; // Record the current state
            }
            else if (state == 2 && currentState != 2) // Boat is online
            {
                //tbCons.Text = tbCons.Text + ("\nONLINE");

                // Update the header
                lTitle.Content = "Boat Online";
                iState.Source = imsBoatOnline;
                ColorAnimation animation = new ColorAnimation(); // Animate the header color change
                animation.Duration = defAnimDuration;
                animation.To = Color.FromRgb(1, 87, 155);
                // Choose the right color based on what state we are coming from
                if (currentState == 0) { animation.From = Color.FromRgb(183, 28, 28); }
                else { animation.From = Color.FromRgb(230, 81, 0); }
                gState.Background.BeginAnimation(SolidColorBrush.ColorProperty, animation);
                mwMain.WindowTitleBrush = new SolidColorBrush(Color.FromRgb(1, 87, 155));

                // Enable servo overriding
                sServOverride.IsEnabled = true;

                // Enable the command input bar
                tbConsInput.IsEnabled = true;

                // Enable the console
                bConsSend.IsEnabled = true;
                sCons.IsEnabled = true;

                currentState = state; // Record the current state
            }

            
        }

        private void myPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            // Each time data is recieved on the port, save it to a buffer.
            // If that buffer contains a ";", parse/execute the completed message
            if (myPort.IsOpen) // First check that the port state has not changed in the meantime
            {
                string newContent = myPort.ReadExisting(); // Read in the incoming data
                xbeeInputBuffer.Append(newContent); // Save the incoming data into the buffer
                if (newContent.Contains(';')) // Check if the incomming buffer contains a semicolon indicating a complete command
                {
                    // Execute the completed command if an entire string has been recieved
                    int searchIndex = 0;
                    string bufferString = xbeeInputBuffer.ToString();
                    int occuranceIndex = bufferString.IndexOf(';');
                    while (occuranceIndex != -1 && searchIndex < bufferString.Length)
                    {
                        int substringLength = occuranceIndex - searchIndex;
                        if (substringLength != 1)
                        {
                            string command = bufferString.Substring(searchIndex, substringLength); // string to hold the completed command
                            try // Parse the message
                            {
                                string code = command.Substring(0, 2);
                                string data = command.Substring(2);
                                consPrintln(code + ": " + data);
                                // Execute the completed message on the UI thread
                                executeTransmission(code, data);
                            }
                            catch (System.ArgumentOutOfRangeException) { } // Problem taking the substring, may be an empty/malformed message
                        }
                        searchIndex = occuranceIndex + 1;
                        occuranceIndex = bufferString.IndexOf(';', searchIndex);
                    }

                    xbeeInputBuffer.Clear(); // Clear the input buffer
                    xbeeInputBuffer.Append(bufferString.Substring(searchIndex)); // If there are any leftever characters, save them back to the input buffer
                }
                else if (xbeeInputBuffer.ToString().Contains("OK") && portScanning == true)
                {
                    // An XBee has responded, indicating that the correct port has been found
                    portScanning = false; // Stop scanning for ports in the timer
                    portTimer.Interval = TimeSpan.FromMilliseconds(3000); // Increase the timer to 3 seconds to reduce background computations.
                    // It's not necessary to respond to XBee unplugged messages "that quickly"
                    //consPrintln("\n\n--- Connected to XBee ---\n");
                    Dispatcher.Invoke((Action)delegate () {
                        if (currentState != 2)
                        {
                            setState(1);
                        }
                        else
                        {
                            lSubTitle.Content = "XBee Connected";
                        }
                    }); // Update the GUI to show that an XBee is present
                    myPort.WriteLine("ATCN"); // Take the XBee out of command mode
                    xbeeInputBuffer.Clear(); // Clear the input buffer (may lose some applicable text but oh well)
                }
            }
        }

        private void executeTransmission(String code, String data) // Take action based on what the boat sends
        {
            if (data != null && !data.Equals(""))
            {
                if (code.Equals("00")) // The boat is communicating state or requesting a response
                {
                    if (data.Equals("0") || data.Equals("1") || data.Equals("2"))
                    {
                        boatHeartbeatTimer.Stop();
                        boatHeartbeatTimer.Start();
                    }

                    if (data.Equals("0") && boatMode != 0)
                    {
                        // Mega is online but is experiencing an error
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            setState(2);
                            lMode.Content = "Critical Error";
                            iMode.Source = imsBoatError;
                            gMode.Background = brushItemError;
                        });
                        boatMode = 0;
                    }
                    else if (data.Equals("1") && boatMode != 1)
                    {
                        // Mega is online and under RC control
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            setState(2);
                            lMode.Content = "Remote Control";
                            iMode.Source = imsBoatRC;
                            gMode.Background = brushItem;
                        });
                        boatMode = 1;
                    }
                    else if (data.Equals("2") && boatMode != 2)
                    {
                        // Mega is online and under RPi control
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            setState(2);
                            lMode.Content = "Autopilot";
                            iMode.Source = imsBoatAutopilot;
                            gMode.Background = brushItem;
                        });
                        boatMode = 2;
                    }
                    else if (data.Equals("?"))
                    {
                        // Mega is asking if XBee is online
                        sendTransmission("00", "1");
                    }
                    if (data.Equals("0") && boatMode == 0 && modeMuted == false) player.Play();
                }
                else if (code.Equals("09")) // RPi state
                {
                    // Update the UI accordingly, but only on the first run through
                    if (data.Equals("0") && rPiState != 0)
                    {
                        // RPi is offline
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            lRPi.Content = "Offline";
                            gRPi.Background = brushItemError;
                        });
                        rPiState = 0;
                    }
                    else if (data.Equals("1") && rPiState != 1)
                    {
                        // RPi is online
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            lRPi.Content = "Online";
                            gRPi.Background = brushItem;
                        });
                        rPiState = 1;
                    }
                    else if (data.Equals("2") && rPiState != 2)
                    {
                        // RPi is experiencing an error
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            lRPi.Content = "Critical Error";
                            gRPi.Background = brushItemError;
                        });
                        rPiState = 2;
                    }
                    if (rPiState == 0 && rPiMuted == false) player.Play();
                }
                else if (code.Equals("GP")) // GPS
                {
                    if (data.Equals("0") && GPSFixed != false)
                    {
                        // GPS no fix
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            lGPS.Content = "No Fix";
                            iGPS.Source = imsGPSNoFix;
                            gGPS.Background = brushItemError;
                        });
                        GPSFixed = false;
                    }
                    else if (data.Equals("1"))
                    {
                        // GPS fix
                        Dispatcher.Invoke((Action)delegate () // Update the UI
                        {
                            lGPS.Content = data;
                            iGPS.Source = imsGPSFix;
                            gGPS.Background = brushItem;
                        });

                        if (GPSFixed != true) // Update the UI only if this is the first time going 
                        {
                            GPSFixed = true;
                        }
                    }
                }
                else if (code.Equals("CP"))
                {
                    // Compass
                    Dispatcher.Invoke((Action)delegate () // Update the UI
                    {
                        lCompass.Content = data + "°";
                        gCompass.Background = brushItem;
                    });
                }
                else if (code.Equals("WV"))
                {
                    // Wind Vane
                    Dispatcher.Invoke((Action)delegate () // Update the UI
                    {
                        lWind.Content = data + "°";
                        gWind.Background = brushItem;
                    });
                }
                else if (code.Equals("TM"))
                {
                    // Temperature
                    Dispatcher.Invoke((Action)delegate () // Update the UI
                    {
                        lTemp.Content = data + "°C";
                        gTemp.Background = brushItem;
                        Int32.TryParse(data, out int temp); // Convert the temperature string to an integer
                        if (temp > 40) gTemp.Background = brushItemError;
                    });
                }
            }
        }


        private void sendTransmission(string code, string message) // Send a transmission to the Mega using the defined communication format
        {
            if (myPort != null && myPort.IsOpen) // Only write if the port is open (xbee connected)
            {
                myPort.Write(code + message + ";");
                if (currentState == 2) // If the boat is currently connected, also assume the command is recieved by the boat and update the UI
                {
                    if (code.Equals("03"))
                    {
                        // If the message is about overriding the remote control
                        // Also update the GUI accordingly
                        // This helps ensure that the GUI only updates if the command has been sent
                        // Or if the command was send through the console
                        if (message.Equals("1"))
                        {
                            sServOverride.IsChecked = false;
                            grServos.Visibility = Visibility.Collapsed;
                            sWinch.IsEnabled = false;
                            sRudder.IsEnabled = false;
                        }
                        else
                        {
                            sServOverride.IsChecked = true;
                            grServos.Visibility = Visibility.Visible;
                            sWinch.IsEnabled = true;
                            sRudder.IsEnabled = true;

                            sWinch.Value = 0;
                            sRudder.Value = 90;
                        }
                    }
                    else if (message.Equals("?0")) { // If the command is to stop updating a sensor, update the UI to reflect that
                        if (code.Equals("CP"))
                        {
                            gCompass.Background = brushItemError;
                            lCompass.Content = "Reporting Disabled";
                        }
                        else if (code.Equals("WV"))
                        {
                            gWind.Background = brushItemError;
                            lWind.Content = "Reporting Disabled";
                        }
                        else if (code.Equals("GP"))
                        {
                            gGPS.Background = brushItemError;
                            lGPS.Content = "Reporting Disabled";
                        }
                        else if (code.Equals("TM"))
                        {
                            gTemp.Background = brushItemError;
                            lTemp.Content = "Reporting Disabled";
                        }
                    }
                }
            }
        }

        private void consPrintln(string message)
        {
            consPrint(message + "\n");
        }

        private void consPrint(string message) // Write to the in app console, but only if it is enabled
        {
            if (consOutputEnabled)
            {
                consoleOutputBuffer.Append(message);

                if (consoleOutputBuffer.ToString().Length > 1000)
                {
                    consoleOutputBuffer.Remove(0, consoleOutputBuffer.ToString().Length - 1000);
                }

                Dispatcher.Invoke((Action)delegate ()
                {
                    tbCons.Text = consoleOutputBuffer.ToString();
                    svCons.ScrollToBottom();
                });
            }

        }

        private void sServOverride_Checked(object sender, RoutedEventArgs e) // Override the servo by sending the disable message
        {
            sendTransmission("03", "" + 0);
        }

        private void sServOverride_Unchecked(object sender, RoutedEventArgs e) // Restore the servo by sending the enable message
        {
            sendTransmission("03", "" + 1);
        }

        private void sWinch_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) // Update the winch
        {
            int iDegree = (int)sWinch.Value;
            tbWinch.Text = iDegree + "% sheeted in"; // Change the winch label
            iDegree = (iDegree) * (180) / (100);
            // Only send if over 2 degree difference to prevent overloading serial comms
            if ((Math.Abs(lastWinch - iDegree) > 2 || iDegree == 0 || iDegree == 100) && myPort != null && myPort.IsOpen)
            {
                sendTransmission("SW", "" + iDegree);
                lastWinch = iDegree;
            }
        }

        private void sRudder_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e) // Update the rudder
        {
            int iDegree = (int)sRudder.Value;
            // Change the rudder labels based on left/right/center
            if (iDegree == 90)
            {
                tbRudder.Text = "Rudders centered";
            }
            else if (iDegree < 90)
            {
                tbRudder.Text = (90 - iDegree) + "° to the left";
            }
            else
            {
                tbRudder.Text = (iDegree - 90) + "° to the right";
            }
            // Only send if over 2 degree difference to prevent overloading serial comms
            if ((Math.Abs(lastWinch - iDegree) > 2 || iDegree == 0 || iDegree == 180 || iDegree == 90) && myPort != null && myPort.IsOpen)
            {
                sendTransmission("SR", "" + iDegree);
                lastRudder = iDegree;
            }
        }

        private void bConsSend_Click(object sender, RoutedEventArgs e)
        {
            sendConsoleMessage();
        }

        private void sendConsoleMessage()
        {
            // Method used to send the message contained in the console textbox
            // Invoked by the button and the enter key
            String message = tbConsInput.Text;
            if (!tbConsInput.ToString().Substring(tbConsInput.ToString().Length - 1).Equals(";"))
                message = message + ";"; // Append a semicolon if it doesn't already have one
            StringBuilder command = new StringBuilder();
            foreach (char c in message)
            {
                // this loop is required as more than one message may be sent at once/partial messages, etc.

                if (c == ';')
                {
                    // Parse the message
                    try
                    {
                        string code = command.ToString().Substring(0, 2);
                        string data = command.ToString().Substring(2);

                        sendTransmission(code, data);
                        consPrintln("> " + message); // Print message to console output
                    }
                    catch (System.ArgumentOutOfRangeException)
                    {
                        consPrintln("> Message malformed. Not sent.");
                    }
                    command.Clear(); // Clear the command buffer
                }
                else command.Append(c);
            }

            tbConsInput.Text = command.ToString();
            tbConsInput.Focus();
        }

        private void tbConsInput_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                sendConsoleMessage();
            }
        }

        private void sCons_Checked(object sender, RoutedEventArgs e) // Enable the in app console
        {
            tbCons.Foreground = new SolidColorBrush(Color.FromRgb(255, 255, 255));
            consOutputEnabled = true;
            tbCons.Text = "";
            sWinch_ValueChanged(null, null);
            sRudder_ValueChanged(null, null);
        }

        private void sCons_Unchecked(object sender, RoutedEventArgs e) // Disable the in app console
        {
            consoleOutputBuffer.Clear();
            tbCons.Foreground = new SolidColorBrush(Color.FromArgb(80, 255, 255, 255));
            consOutputEnabled = false;
        }

        private void imMode_Tap()
        {
            if (modeMuted == false)
            {
                modeMuted = true;
                imMode.Source = imsMuted;
            }
            else
            {
                modeMuted = false;
                imMode.Source = imsUnMuted;
            }
        }
        private void imRPi_Tap()
        {
            if (rPiMuted == false)
            {
                rPiMuted = true;
                imRPi.Source = imsMuted;
            }
            else
            {
                rPiMuted = false;
                imRPi.Source = imsUnMuted;
            }
        }


        private void imMode_Tap(object sender, MouseButtonEventArgs e)
        {
            imMode_Tap();
        }

        private void imMode_Tap(object sender, TouchEventArgs e)
        {
            imMode_Tap();
        }

        private void imMode_Tap(object sender, StylusEventArgs e)
        {
            imMode_Tap();
        }

        private void imRPi_Tap(object sender, TouchEventArgs e)
        {
            imRPi_Tap();
        }

        private void imRPi_Tap(object sender, MouseButtonEventArgs e)
        {
            imRPi_Tap();
        }

        private void imRPi_Tap(object sender, StylusEventArgs e)
        {
            imRPi_Tap();
        }
    }
}
