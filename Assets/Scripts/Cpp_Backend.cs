using UnityEngine;
using System.Runtime.InteropServices;

public class PCSpecsManager : MonoBehaviour
{
    [DllImport("backend")]
    private static extern System.IntPtr GetPCSpecs();

    private void Start()
    {
        System.IntPtr specsPtr = GetPCSpecs();
        string specs = Marshal.PtrToStringAnsi(specsPtr);
        Debug.Log("PC Specs:\n" + specs);
    }
}
