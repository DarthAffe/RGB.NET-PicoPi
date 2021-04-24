using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using HidSharp;

//TODO DarthAffe 24.04.2021: Replace with RGB.NET PicoPiSDK once it's merged to main
namespace RGB.NET.Devices.PicoPi
{
    public class PicoPiSDK : IDisposable
    {
        #region Constants

        public const int VENDOR_ID = 0x1209;
        public const int HID_BULK_CONTROLLER_PID = 0x2812;

        private const byte COMMAND_CHANNEL_COUNT = 0x01;
        private const byte COMMAND_LEDCOUNTS = 0x0A;
        private const byte COMMAND_PINS = 0x0B;
        private const byte COMMAND_ID = 0x0E;
        private const byte COMMAND_VERSION = 0x0F;
        private const byte COMMAND_UPDATE = 0x01;
        private const byte COMMAND_UPDATE_BULK = 0x02;

        #endregion

        #region Properties & Fields

        private readonly HidDevice _hidDevice;
        private readonly HidStream _hidStream;

        private readonly byte[] _hidSendBuffer;
        private readonly byte[] _bulkSendBuffer;

        private int _bulkTransferLength = 0;

        public string Id { get; }
        public int Version { get; }
        public IReadOnlyList<(int channel, int ledCount, int pin)> Channels { get; }

        #endregion

        #region Constructors

        public PicoPiSDK(HidDevice device)
        {
            this._hidDevice = device;

            _hidSendBuffer = new byte[_hidDevice.GetMaxOutputReportLength() - 1];

            _hidStream = _hidDevice.Open();

            Id = GetId();
            Version = GetVersion();
            Channels = new ReadOnlyCollection<(int channel, int ledCount, int pin)>(GetChannels().ToList());

            _bulkSendBuffer = new byte[(Channels.Sum(c => c.ledCount + 1) * 3) + 5];
        }

        #endregion

        #region Methods

        public void SetLedCounts(params (int channel, int ledCount)[] ledCounts)
        {
            byte[] data = new byte[Channels.Count + 2];
            data[1] = COMMAND_LEDCOUNTS;
            foreach ((int channel, int ledCount, _) in Channels)
                data[channel + 1] = (byte)ledCount;

            foreach ((int channel, int ledCount) in ledCounts)
                data[channel + 1] = (byte)ledCount;

            SendHID(data);
        }

        public void SetPins(params (int channel, int pin)[] pins)
        {
            byte[] data = new byte[Channels.Count + 2];
            data[1] = COMMAND_PINS;
            foreach ((int channel, _, int pin) in Channels)
                data[channel + 1] = (byte)pin;

            foreach ((int channel, int pin) in pins)
                data[channel + 1] = (byte)pin;

            SendHID(data);
        }

        private string GetId()
        {
            SendHID(0x00, COMMAND_ID);
            return ConversionHelper.ToHex(Read().Skip(1).Take(8).ToArray());
        }

        private int GetVersion()
        {
            SendHID(0x00, COMMAND_VERSION);
            return Read()[1];
        }

        private IEnumerable<(int channel, int ledCount, int pin)> GetChannels()
        {
            SendHID(0x00, COMMAND_CHANNEL_COUNT);
            int channelCount = Read()[1];

            for (int i = 1; i <= channelCount; i++)
            {
                SendHID(0x00, (byte)((i << 4) | COMMAND_LEDCOUNTS));
                int ledCount = Read()[1];

                SendHID(0x00, (byte)((i << 4) | COMMAND_PINS));
                int pin = Read()[1];

                yield return (i, ledCount, pin);
            }
        }

        public void SendHidUpdate(in Span<byte> data, int channel, int chunk, bool update)
        {
            if (data.Length == 0) return;

            Span<byte> sendBuffer = _hidSendBuffer;
            sendBuffer[0] = 0x00;
            sendBuffer[1] = (byte)((channel << 4) | COMMAND_UPDATE);
            sendBuffer[2] = update ? (byte)1 : (byte)0;
            sendBuffer[3] = (byte)chunk;
            data.CopyTo(sendBuffer.Slice(4, data.Length));
            SendHID(_hidSendBuffer);
        }

        private void SendHID(params byte[] data) => _hidStream.Write(data);

        private byte[] Read() => _hidStream.Read();

        public void Dispose()
        {
            _hidStream.Dispose();
        }

        #endregion
    }

    /// <summary>
    /// Contains helper methods for converting things.
    /// </summary>
    public static class ConversionHelper
    {
        #region Methods

        // Source: https://web.archive.org/web/20180224104425/https://stackoverflow.com/questions/623104/byte-to-hex-string/3974535
        /// <summary>
        /// Converts an array of bytes to a HEX-representation.
        /// </summary>
        /// <param name="bytes">The array of bytes.</param>
        /// <returns>The HEX-representation of the provided bytes.</returns>
        public static string ToHex(params byte[] bytes)
        {
            char[] c = new char[bytes.Length * 2];

            for (int bx = 0, cx = 0; bx < bytes.Length; ++bx, ++cx)
            {
                byte b = ((byte)(bytes[bx] >> 4));
                c[cx] = (char)(b > 9 ? b + 0x37 : b + 0x30);

                b = ((byte)(bytes[bx] & 0x0F));
                c[++cx] = (char)(b > 9 ? b + 0x37 : b + 0x30);
            }

            return new string(c);
        }

        // Source: https://web.archive.org/web/20180224104425/https://stackoverflow.com/questions/623104/byte-to-hex-string/3974535
        /// <summary>
        /// Converts the HEX-representation of a byte array to that array.
        /// </summary>
        /// <param name="hexString">The HEX-string to convert.</param>
        /// <returns>The correspondending byte array.</returns>
        public static byte[] HexToBytes(ReadOnlySpan<char> hexString)
        {
            if ((hexString.Length == 0) || ((hexString.Length % 2) != 0))
                return Array.Empty<byte>();

            byte[] buffer = new byte[hexString.Length / 2];
            for (int bx = 0, sx = 0; bx < buffer.Length; ++bx, ++sx)
            {
                // Convert first half of byte
                char c = hexString[sx];
                buffer[bx] = (byte)((c > '9' ? (c > 'Z' ? ((c - 'a') + 10) : ((c - 'A') + 10)) : (c - '0')) << 4);

                // Convert second half of byte
                c = hexString[++sx];
                buffer[bx] |= (byte)(c > '9' ? (c > 'Z' ? ((c - 'a') + 10) : ((c - 'A') + 10)) : (c - '0'));
            }

            return buffer;
        }

        #endregion
    }
}
