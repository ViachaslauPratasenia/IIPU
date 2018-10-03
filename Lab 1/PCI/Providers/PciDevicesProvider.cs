using System.Collections.Generic;
using System.Management;
using PCI.Core;
using PCI.Database;
using PCI.Models;

namespace PCI.Providers
{
    public class PciDevicesProvider
    {
        private readonly DeviceParser _deviceParser;

        public PciDevicesProvider()
        {
            _deviceParser = new DeviceParser(new PciDatabase());
        }

        public List<Device> GetDevices()
        {
            var searcher = new ManagementObjectSearcher(    //класс для запроса
                scope: new ManagementScope(),               //контекст поиска
                query: new SelectQuery("SELECT * FROM Win32_PnPEntity WHERE DeviceID LIKE 'PCI%'")
            );

            var devices = new List<Device>();

            foreach (var deviceInfo in searcher.Get())  //возвращает результат запроса
            {
                var device = _deviceParser.Parse(deviceInfo);
                devices.Add(device);
            }

            return devices;
        }
    }
}
