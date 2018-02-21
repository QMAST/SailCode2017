using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System;
using System.IO.Ports;


namespace QMAST
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow
    {
        SerialPort myPort;
        private StringBuilder xbeeInputBuffer = new StringBuilder();
        private StringBuilder consoleOutputBuffer = new StringBuilder();

        Brush brushItemError = new SolidColorBrush(Color.FromArgb(204, 183, 28, 28));
        Brush brushItem = new SolidColorBrush(Color.FromArgb(127, 0, 0, 0));

        int lastRudder = -2;
        int lastWinch = -2;

        int currentState = 0;

        System.Windows.Threading.DispatcherTimer boatHeartbeatTimer = null;

        System.Windows.Threading.DispatcherTimer portTimer = null;
        string[] detectedPorts;
        int nextPortIndex;
        bool portScanning = false;

        bool consOutputEnabled = false;

        public MainWindow()
        {
            InitializeComponent();
            
            portTimer = new System.Windows.Threading.DispatcherTimer();
            portTimer.Tick += new EventHandler(portTimer_Tick);
            portTimer.Interval = TimeSpan.FromMilliseconds(1500);
            portTimer.Start();

            portTimer_Tick(null, null);

            boatHeartbeatTimer = new System.Windows.Threading.DispatcherTimer();
            boatHeartbeatTimer.Interval = TimeSpan.FromMilliseconds(6000);
            boatHeartbeatTimer.Tick += new EventHandler(boatHeartbeatTimer_Tick);
        }

        private void portTimer_Tick(object sender, EventArgs e)
        {
            if ((myPort != null && myPort.IsOpen) && !portScanning)
            {

            }
            else
            {
                portTimer.Interval = TimeSpan.FromMilliseconds(1250); // Speed up the interval as much as possible, without missing the response from an XBee
                Dispatcher.Invoke((Action)delegate ()
                {
                    setState(0); // Set the state to reflect no XBee connected
                });
                portScanning = true;

                if (detectedPorts == null || detectedPorts.Length == 0 || nextPortIndex >= detectedPorts.Length)
                {
                    // Get a list of serial port names.
                    detectedPorts = SerialPort.GetPortNames();
                    if (detectedPorts.Length != 0)
                    {
                        // If the system has serial devices connected, try to connect to them
                        nextPortIndex = 0;
                        checkPortsForXBee();
                    }
                }
                else
                {
                    // The polled serial device did not respond in time, try another device
                    checkPortsForXBee();
                }
            }
        }

        private void boatHeartbeatTimer_Tick(object sender, EventArgs e)
        {
            Dispatcher.Invoke((Action)delegate ()
            {
                setState(1);
            });
        }

        private void checkPortsForXBee()
        {
            bool portSelected = false;
            while (portSelected == false)
            {
                if (detectedPorts == null || detectedPorts.Length == 0 || nextPortIndex >= detectedPorts.Length)
                {
                    // All ports have been cycled through. 
                    // The next portResponseTimer_Tick will refresh detectedPorts 
                    portSelected = true;
                    detectedPorts = null;
                }
                else
                {
                    // Try connecting to the next available serial port
                    if (myPort != null && myPort.IsOpen) myPort.Close();

                    try
                    {
                        //consPrintln("Trying port " + detectedPorts[nextPortIndex]);
                        myPort = new SerialPort(detectedPorts[nextPortIndex], 57600);
                        myPort.Open();
                        myPort.DataReceived += new SerialDataReceivedEventHandler(myPort_DataReceived);
                        myPort.Write("+++");
                        portSelected = true;
                    }
                    catch (System.IO.IOException)
                    {
                        //consPrintln(detectedPorts[nextPortIndex] + "is in an invalid state.");
                    }
                    catch (System.UnauthorizedAccessException)
                    {
                        //consPrintln(detectedPorts[nextPortIndex] + " is busy. Access is denied.");
                    }
                    catch (System.InvalidOperationException)
                    {
                        //consPrintln(detectedPorts[nextPortIndex] + " already open!");
                    }

                    nextPortIndex++;
                }
            }
        }

        private void setState(int state)
        {
            if (boatHeartbeatTimer != null) boatHeartbeatTimer.Stop();

            if ((state == 0 || state == 1) && (currentState < 2))
            {
                lTitle.Content = "Boat Offline";
                sServOverride.IsChecked = false;
                sServOverride.IsEnabled = false;
                gMode.Background = brushItemError;
                gRPi.Background = brushItemError;
                gCompass.Background = brushItemError;
                gWind.Background = brushItemError;
                gGPS.Background = brushItemError;
            }

            if (state == 0 && currentState != 0)
            {
                // XBee disconnected
                lSubTitle.Content = "XBee Disconnected";
                iState.Source = new BitmapImage(new Uri(@"/alert-circle.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(183, 28, 28));
                currentState = 0;
                tbConsInput.IsEnabled = false;
                bConsSend.IsEnabled = false;
                sCons.IsEnabled = false;
                sCons.IsChecked = false;
            }
            else if (state == 1 && currentState != 1)
            {
                // Boat offline, XBee connected
                lSubTitle.Content = "XBee Connected";
                iState.Source = new BitmapImage(new Uri(@"/lan-pending.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(230, 81, 0));
                currentState = 1;
                tbConsInput.IsEnabled = true;
                bConsSend.IsEnabled = true;
                sCons.IsEnabled = true;
            }
            else if (state == 2 && currentState != 2)
            {
                // Boat online
                lTitle.Content = "Boat Online";
                iState.Source = new BitmapImage(new Uri(@"/lan-connect.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(1, 87, 155));
                sServOverride.IsEnabled = true;
                boatHeartbeatTimer.Start(); // Start boat offline countdown
                currentState = 2;
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
                                Dispatcher.Invoke((Action)delegate ()
                                {
                                    executeTransmission(code, data);
                                });
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
                    myPort.WriteLine("ATCN"); // Take the XBee out of command mode
                    //consPrintln("\n\n--- Connected to XBee ---\n");
                    Dispatcher.Invoke((Action)delegate () { setState(1); }); // Update the GUI to show that an XBee is present
                    xbeeInputBuffer.Clear(); // Clear the input buffer (may lose some applicable text but oh well)
                }
            }
        }

        private void executeTransmission(String code, String data)
        {
            if (code.Equals("00"))
            {
                if (data.Equals("0"))
                {
                    // Mega is online but errored
                    setState(2);
                    lMode.Content = "Critical Error";
                    iMode.Source = new BitmapImage(new Uri(@"/alert-circle.png", UriKind.Relative));
                    gMode.Background = brushItemError;
                }
                else if (data.Equals("1"))
                {
                    // Mega is online and under RC control
                    setState(2);
                    lMode.Content = "Remote Control";
                    iMode.Source = new BitmapImage(new Uri(@"/gamepad-variant.png", UriKind.Relative));
                    gMode.Background = brushItem;
                }
                else if (data.Equals("2"))
                {
                    // Mega is online and under RPi control
                    setState(2);
                    lMode.Content = "Autopilot";
                    iMode.Source = new BitmapImage(new Uri(@"/infinity.png", UriKind.Relative));
                    gMode.Background = brushItem;
                }
                else if (data.Equals("?"))
                {
                    sendTransmission("00", "1");
                }
            }
            else if (code.Equals("09"))
            {
                if (data.Equals("0"))
                {
                    // RPi is offline
                    lRPi.Content = "Offline";
                    gRPi.Background = brushItemError;
                }
                else if (data.Equals("1"))
                {
                    // RPi is online
                    lRPi.Content = "Online";
                    gRPi.Background = brushItem;
                }
            }
            else if (code.Equals("GP"))
            {
                if (data.Equals("0"))
                {
                    // GPS no fix
                    lGPS.Content = "No Fix";
                    iGPS.Source = new BitmapImage(new Uri(@"/map-marker-off.png", UriKind.Relative));
                    gGPS.Background = brushItemError;
                }
                else
                {
                    // GPS fix
                    lGPS.Content = data;
                    iGPS.Source = new BitmapImage(new Uri(@"/map-marker.png", UriKind.Relative));
                    gGPS.Background = brushItem;
                }
            }
            else if (code.Equals("CP"))
            {
                // Compass
                lCompass.Content = data + "°";
                gCompass.Background = brushItem;
            }
            else if (code.Equals("WV"))
            {
                // Wind Vane
                lWind.Content = data + "°";
                gWind.Background = brushItem;
            }
        }


        private void sendTransmission(string code, string message)
        {
            if (myPort != null && myPort.IsOpen)
            {
                myPort.Write(code + message + ";");
                if (currentState == 2)
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
                    }else if(code.Equals("CP") && message.Equals("?0"))
                    {
                        gCompass.Background = brushItemError;
                        lCompass.Content = "Reporting Disabled";
                    }
                    else if (code.Equals("WV") && message.Equals("?0"))
                    {
                        gWind.Background = brushItemError;
                        lWind.Content = "Reporting Disabled";
                    }
                    else if (code.Equals("GP") && message.Equals("?0"))
                    {
                        gGPS.Background = brushItemError;
                        lGPS.Content = "Reporting Disabled";
                    }
                }
            }
        }

        private void consPrintln(string message)
        {
            consPrint(message + "\n");
        }

        private void consPrint(string message)
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

        private void sServOverride_Checked(object sender, RoutedEventArgs e)
        {
            sendTransmission("03", "" + 0);
        }

        private void sServOverride_Unchecked(object sender, RoutedEventArgs e)
        {
            sendTransmission("03", "" + 1);
        }

        private void sWinch_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            int iDegree = (int)sWinch.Value;
            tbWinch.Text = iDegree + "% sheeted in";
            iDegree = (iDegree) * (180) / (100);
            if (Math.Abs(lastWinch - iDegree) > 2 || iDegree == 0 || iDegree == 100)
            {
                sendTransmission("SW", "" + iDegree);
                lastWinch = iDegree;
            }
        }

        private void sRudder_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            int iDegree = (int)sRudder.Value;
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
            if (Math.Abs(lastWinch - iDegree) > 2 || iDegree == 0 || iDegree == 180 || iDegree == 90)
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

        private void sCons_Checked(object sender, RoutedEventArgs e)
        {
            tbCons.Foreground = new SolidColorBrush(Color.FromRgb(255, 255, 255));
            consOutputEnabled = true;
            tbCons.Text = "";
        }

        private void sCons_Unchecked(object sender, RoutedEventArgs e)
        {
            consoleOutputBuffer.Clear();
            tbCons.Foreground = new SolidColorBrush(Color.FromArgb(80,255, 255, 255));
            consOutputEnabled = false;
        }
    }
}
