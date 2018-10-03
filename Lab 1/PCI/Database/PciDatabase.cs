using System;
using System.IO;
using System.Linq;

namespace PCI.Database
{
    public class PciDatabase
    {
        public string[] Data { get; private set; }

        public PciDatabase()
        {
            Initialize();
        }

        private void Initialize()
        {
            try
            {
                Data = File.ReadLines("E:/IIPU-master/PCI/pci.ids.txt").ToArray();
            }catch(Exception e)
            {
                Console.WriteLine("Failed to load ids: " + e.Message);
                Console.ReadKey();
            }
            
        }
    }
}
