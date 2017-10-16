using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Text;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using GMap.NET.WindowsPresentation;

namespace Boat_Control
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        List<GMapMarker> lWaypoints;
        private SerialPort port = new SerialPort("COM3");
        private StringBuilder commBuffer = new StringBuilder();
        public MainWindow()
        {
            InitializeComponent();
            // Set up Serial Comm
            port.DataReceived += new SerialDataReceivedEventHandler(port_DataReceived);
            port.Open();
            

            // Set up map
            gMap.MapProvider = GMap.NET.MapProviders.BingSatelliteMapProvider.Instance;
            GMap.NET.GMaps.Instance.Mode = GMap.NET.AccessMode.ServerAndCache;
            GMap.NET.GMaps.Instance.OptimizeMapDb(null);
            gMap.Position = new GMap.NET.PointLatLng(44.225587, -76.495164);
            gMap.IsEnabled = false;
            gMap.ShowCenter = false;
            gMap.DragButton = MouseButton.Left;

            lWaypoints = new List<GMapMarker>();

            GMapMarker mark1 = new GMapMarker(new GMap.NET.PointLatLng(44.225587, -76.495164));
            lWaypoints.Add(mark1);

            refreshWaypoints();
        }

        private void OnMouseUp(Object sender, MouseButtonEventArgs e)
        {
         
        }

        private void refreshWaypoints()
        {
            gMap.Markers.Clear();
            for (int i = 0; i < lWaypoints.Count; i++)
            {
                lWaypoints[i].Shape = new Ellipse
                {
                    Width = 15,
                    Height = 15,
                    Stroke = Brushes.OrangeRed,
                    StrokeThickness = 5
                };
                gMap.Markers.Add(lWaypoints[i]);
            }
        }

        private void port_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            commBuffer.Append(port.ReadExisting());
            if (commBuffer.ToString().Contains(";"))
            {
                StringBuilder command = new StringBuilder();
                foreach (char c in commBuffer.ToString()){
                    command.Append(c);
                    if (c == ';')
                    {
                        Console.WriteLine(command.ToString());

                        Dispatcher.Invoke((Action)delegate () {
                            tbStatTitle.Text = "Status: " + command.ToString();
                        });

                        command.Clear();
                    }
                }
                commBuffer.Clear();
                commBuffer.Append(command.ToString());
            }
        }

        private void bAddCancel_Click(object sender, RoutedEventArgs e)
        {
            gAddWaypoint.Visibility = Visibility.Collapsed;
            svFeed.Visibility = Visibility.Visible;
            gMap.IsEnabled = false;
        }

        private void bLocAdd_Click(object sender, RoutedEventArgs e)
        {
            gAddWaypoint.Visibility = Visibility.Visible;
            svFeed.Visibility = Visibility.Collapsed;
            gMap.IsEnabled = true;
        }

        private void bMotorL_Click(object sender, RoutedEventArgs e)
        {
            port.Write("h;");
        }

        private void bMotorU_Click(object sender, RoutedEventArgs e)
        {
            port.Write("l;");
        }
    }
}