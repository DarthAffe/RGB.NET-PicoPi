using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace PicoPiConfig
{
    public class Channel : INotifyPropertyChanged
    {
        #region Properties & Fields

        private int _index;
        public int Index
        {
            get => _index;
            set
            {
                _index = value;
                OnPropertyChanged();
            }
        }

        private int _pin;
        public int Pin
        {
            get => _pin;
            set
            {
                _pin = value;
                OnPropertyChanged();
            }
        }

        private int _ledCount;
        public int LedCount
        {
            get => _ledCount;
            set
            {
                _ledCount = value;
                OnPropertyChanged();
            }
        }

        #endregion

        #region Constructors

        public Channel(int index, int pin, int ledCount)
        {
            this.Index = index;
            this.Pin = pin;
            this.LedCount = ledCount;
        }

        #endregion

        #region PropertyChanged

        public event PropertyChangedEventHandler? PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string? propertyName = null)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));

        #endregion
    }
}
