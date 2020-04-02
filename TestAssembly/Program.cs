using System;
using System.Collections.Generic;
using System.Text;
using System.Management;

namespace TestAssembly
{
    class Program
    {
        static void Main(string[] args)
        {
            foreach (var s in args)
            {
                Console.WriteLine(s);
            }
            Console.WriteLine("[+] Process: {0}",Distinguish64or32System());
            Console.WriteLine("[+] Net Vsersion: {0}", Environment.Version.ToString());
        }

        private static string Distinguish64or32System()
        {
            try
            {
                string addressWidth = String.Empty;
                ConnectionOptions mConnOption = new ConnectionOptions();
                ManagementScope mMs = new ManagementScope("//localhost", mConnOption);
                ObjectQuery mQuery = new ObjectQuery("select AddressWidth from Win32_Processor");
                ManagementObjectSearcher mSearcher = new ManagementObjectSearcher(mMs, mQuery);
                ManagementObjectCollection mObjectCollection = mSearcher.Get();
                foreach (ManagementObject mObject in mObjectCollection)
                {
                    addressWidth = mObject["AddressWidth"].ToString();
                }
                return addressWidth;
            }
            catch (Exception ex)
            {
                return String.Empty;
            }
        }
    }
}
