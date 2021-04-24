using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Windows;
using HidSharp;
using RGB.NET.Devices.PicoPi;

namespace PicoPiConfig
{
    public class MainWindowViewModel : INotifyPropertyChanged
    {
        #region Properties & Fields

        private List<PicoPiSDK>? _devices;
        public List<PicoPiSDK>? Devices
        {
            get => _devices;
            set
            {
                _devices = value;
                OnPropertyChanged();
            }
        }

        private PicoPiSDK? _selectedDevice;
        public PicoPiSDK? SelectedDevice
        {
            get => _selectedDevice;
            set
            {
                _selectedDevice = value;
                OnPropertyChanged();
                ReloadChannels();
            }
        }

        private List<Channel>? _channels;
        public List<Channel>? Channels
        {
            get => _channels;
            set
            {
                _channels = value;
                OnPropertyChanged();
            }
        }

        #endregion

        #region Commands

        private ActionCommand? _reloadCommand;
        public ActionCommand ReloadCommand => _reloadCommand ??= new ActionCommand(Reload);

        private ActionCommand? _savePinsCommand;
        public ActionCommand SavePinsCommand => _savePinsCommand ??= new ActionCommand(SavePins);

        private ActionCommand? _saveLedCountsCommand;
        public ActionCommand SaveLedCountsCommand => _saveLedCountsCommand ??= new ActionCommand(SaveLedCounts);

        #endregion

        #region Constructors

        public MainWindowViewModel()
        {
            Reload();
        }

        #endregion

        #region Methods

        private void Reload()
        {
            if (Devices != null)
                foreach (PicoPiSDK sdk in Devices) sdk.Dispose();

            Devices = DeviceList.Local.GetHidDevices(PicoPiSDK.VENDOR_ID, PicoPiSDK.HID_BULK_CONTROLLER_PID).Select(d => new PicoPiSDK(d)).ToList();
            SelectedDevice = Devices.FirstOrDefault();
        }

        private void ReloadChannels()
        {
            Channels = SelectedDevice?.Channels.Select(c => new Channel(c.channel, c.pin, c.ledCount)).ToList();
        }

        private void SavePins()
        {
            if (Channels == null) return;

            try
            {
                SelectedDevice?.SetPins(Channels.Select(c => (c.Index, c.Pin)).ToArray());
                MessageBox.Show("Pins saved!\r\nRestart/Reconnect the device for the changes to take effect.", "Pins", MessageBoxButton.OK);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed saving pins:\r\n{ex.StackTrace}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void SaveLedCounts()
        {
            if (Channels == null) return;

            try
            {
                SelectedDevice?.SetLedCounts(Channels.Select(c => (c.Index, c.LedCount)).ToArray());
                MessageBox.Show("Led-counts saved!", "Led-counts", MessageBoxButton.OK);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed saving led-counts:\r\n{ex.StackTrace}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        #endregion

        #region PropertyChanged

        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

        #endregion
    }
}
