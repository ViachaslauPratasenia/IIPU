using System;
using PCI.Providers;

namespace PCI
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var provider = new PciDevicesProvider();
            try
            {
                foreach (var device in provider.GetDevices())
                    Console.WriteLine("{0, -5} {1, -40} {2, -5} {3, -50}",  
                        device.VendorId, 
                        device.VendorName, 
                        device.DeviceId, 
                        device.DeviceName);
                Console.ReadKey();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
            }
        }
    }
}
