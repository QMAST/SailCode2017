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

        Brush brushItemError = new SolidColorBrush(Color.FromArgb(204, 183, 28, 28));
        Brush brushItem = new SolidColorBrush(Color.FromArgb(127, 0, 0, 0));

        int lastRudder = -2;
        int lastWinch = -2;

        System.Windows.Threading.DispatcherTimer boatHeartbeatTimer = null;


        public MainWindow()
        {
            InitializeComponent();
            selectPort();

            System.Windows.Threading.DispatcherTimer dispatcherTimer = new System.Windows.Threading.DispatcherTimer();
            dispatcherTimer.Tick += new EventHandler(portTimer_Tick);
            dispatcherTimer.Interval = new TimeSpan(0, 0, 3);
            dispatcherTimer.Start();


            boatHeartbeatTimer = new System.Windows.Threading.DispatcherTimer();
            boatHeartbeatTimer.Interval = TimeSpan.FromMilliseconds(6000);
            boatHeartbeatTimer.Tick += new EventHandler(boatHeartbeatTimer_Tick);
        }

        private void portTimer_Tick(object sender, EventArgs e)
        {
            //do this every five seconds....
            if (myPort != null && myPort.IsOpen)
            {

            }
            else
            {
                setState(0);
                selectPort();
            }
        }

        private void boatHeartbeatTimer_Tick(object sender, EventArgs e)
        {
            Dispatcher.Invoke((Action)delegate ()
            {
                setState(1);
            });
        }


        private void selectPort()
        {
            // Get a list of serial port names.
            string[] ports = SerialPort.GetPortNames();

            if (ports.Length == 1)
            {
                // If there is only one serial port, try connecting to it
                if (myPort != null && myPort.IsOpen) myPort.Close();
                try
                {
                    myPort = new SerialPort(ports[0], 115200);
                    myPort.Open();
                    consPrintln("\n\nConnected to " + ports[0]);
                    setState(1);
                    myPort.DataReceived += new SerialDataReceivedEventHandler(myPort_DataReceived);
                }
                catch (System.IO.IOException e)
                {
                    consPrintln(ports[0] + "is in an invalid state.");
                }
                catch (System.UnauthorizedAccessException)
                {
                    consPrintln(ports[0] + " is busy. Access is denied.");
                }
                catch (System.InvalidOperationException)
                {
                    consPrintln(ports[0] + " already open!");
                }
            }
            else if (ports.Length > 0)
            {
                consPrintln("The following serial ports were found:");

                // Display each port name to the console.
                foreach (string port in ports)
                {
                    consPrintln(port);
                }

                consPrintln("End Search");
            }
            else
            {
                consPrintln("XBee not found");
            }
        }

        private void setState(int state)
        {
            if(boatHeartbeatTimer!= null) boatHeartbeatTimer.Stop();
            if (state == 0)
            {
                // XBee disconnected
                lSubTitle.Content = "XBee Disconnected";
                iState.Source = new BitmapImage(new Uri(@"/alert-circle.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(183, 28, 28));
            }
            else if (state == 1)
            {
                // Boat offline, XBee connected
                lSubTitle.Content = "XBee Connected";
                iState.Source = new BitmapImage(new Uri(@"/lan-pending.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(230, 81, 0));
            }
            else
            {
                // Boat online
                lTitle.Content = "Boat Online";
                lSubTitle.Content = "XBee Connected";
                iState.Source = new BitmapImage(new Uri(@"/lan-connect.png", UriKind.Relative));
                gState.Background = new SolidColorBrush(Color.FromRgb(1, 87, 155));
                sServOverride.IsEnabled = true;
                boatHeartbeatTimer.Start(); // Start boat offline countdown
            }

            if (state == 0 || state == 1)
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
        }

        private void myPort_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            // Each time data is recieved on the port, save it to a buffer
            xbeeInputBuffer.Append(myPort.ReadExisting());
            if (xbeeInputBuffer.ToString().Contains(";"))
            {
                // Execute the completed command if an entire string has been recieved
                StringBuilder command = new StringBuilder();
                foreach (char c in xbeeInputBuffer.ToString())
                {
                    // Build the command character by character until a semicolon is reached
                    // this loop is required as more than one message may be recieved at once/partial messages, etc.

                    if (c == ';')
                    {
                        // Parse the message
                        try
                        {
                            string code = command.ToString().Substring(0, 2);
                            string data = command.ToString().Substring(2);
                            // Execute the completed message on the UI thread
                            Dispatcher.Invoke((Action)delegate ()
                            {
                                consPrintln("Code: " + code + ", data: " + data);
                                executeTransmission(code, data);
                            });
                        }
                        catch (System.ArgumentOutOfRangeException) { }
                        command.Clear(); // Clear the command buffer
                    }
                    else command.Append(c);
                }
                xbeeInputBuffer.Clear(); // Clear the input buffer
                xbeeInputBuffer.Append(command.ToString()); // If there are any leftever characters, save them back to the input buffer
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
            if(myPort != null && myPort.IsOpen)
            {
                myPort.Write(code + message + ";");

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
            }
        }

        private void consPrintln(string message)
        {
            consPrint(message + "\n");
        }

        private void consPrint(string message)
        {

            tbCons.Text = tbCons.Text + message;

            if (tbCons.Text.Length > 100000)
            {
                tbCons.Text = tbCons.Text.Substring(tbCons.Text.Length - 100000);
            }
            svCons.ScrollToBottom();
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
            if(Math.Abs(lastWinch-iDegree) > 2 || iDegree == 0 || iDegree == 100)
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
    }
}
