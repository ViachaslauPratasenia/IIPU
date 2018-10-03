using System.Linq;
using System.Management;
using System.Text.RegularExpressions;
using PCI.Database;
using PCI.Models;

namespace PCI.Core
{
    public class DeviceParser
    {
        private readonly PciDatabase _database; //константа времени выполнения(можно инициалищировать только в конструкторе)

        public DeviceParser(PciDatabase database)
        {
            _database = database;
        }

        public string GetDeviceId(string description)
        {
            var deviceIdRegex = new Regex("DEV_.{4}");  //поиск подстроки, в которой есть DEV и 4 символа id
            var deviceId = deviceIdRegex.Match(description).Value.Substring(4)?.ToLower();  //substring - вся оставшаяся часть после 4 символов

            return deviceId;
        }

        public string GetVendorId(string description)
        {
            var vendorIdRegex = new Regex("VEN_.{4}");
            var vendor = vendorIdRegex.Match(description).Value.Substring(4)?.ToLower();

            return vendor;
        }

        public string GetVendorName(string vendorId)
        {
            var vendorNameRegex = new Regex("^" + vendorId + "  "); //^ - начало строки 
            return _database.Data.FirstOrDefault(s => vendorNameRegex.Match(s).Success)?.Substring(6);  //6 - vendor id + 2 пробела
        }

        public string GetDeviceName(string deviceId)
        {
            var deviceNameRegex = new Regex("^\\t" + deviceId + "  ");  //строки с device id начинаются с табуляции
            return _database.Data.FirstOrDefault(s => deviceNameRegex.Match(s).Success)?.Substring(7);  //device id + 2 пробела + табуляция
        }

        public Device Parse(ManagementBaseObject managmentObject)
        {
            var info = managmentObject["DeviceID"].ToString();  //вся информация об устройстве находится в объекте под именем device id

            var device = new Device
            {
                DeviceId = GetDeviceId(info),
                VendorId = GetVendorId(info)
            };
            device.DeviceName = GetDeviceName(device.DeviceId);
            device.VendorName = GetVendorName(device.VendorId);

            return device;
        }
    }
}
