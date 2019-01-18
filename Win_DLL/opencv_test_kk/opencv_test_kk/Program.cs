using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace opencv_test_kk
{
    class Program
    {
        [DllImport("KTC_opencv_intercept.dll")]     //你生成的.dll 文件名
        private extern static int KTC_image_extract(string input_file_name, int deviation, int threshold_thresh, float k, ref byte out_file_name, ref int count);
        static void Main(string[] args)
        {
            Console.WriteLine("测试Opencv");
            byte[] out_test = new byte[255];
            int count = 0;
            int ret = KTC_image_extract("C:\\Data\\Image_20190103151821226.jpg", 150, 160, 0.60f, ref out_test[0], ref count);
            string strGet = System.Text.Encoding.Default.GetString(out_test, 0, count); //将字节数组转换为字符串
            Console.WriteLine("ret:" + ret);
            if (ret == 1) {
                Console.WriteLine("strGet:" + strGet);
            }
            else if (ret == -1) {
                Console.WriteLine("first third not exist!please reset value k!");
            }
            else if (ret == -2) {
                Console.WriteLine("second or fouth not exist! please reset deviation!");
            }
            else if (ret == 0) {
                Console.WriteLine("读取错误！");
            }
            
            Console.Read();
        }
    }
}
