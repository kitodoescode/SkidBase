# SkidBase

This is a roblox exploit base built from the leaked "Arctic-Internal" source, functions were added from scratch along with the tphandler.

## Some Info

- v1.0.1
- Updated for version-82f8ee8d17124507
- 26 sUNC
- 80 UNC (disabled/commented out till further development, thus making it 35 UNC)
- Stable TPHandler

## Note
Right click the "lib.rar" in Dependencies/cryptopp and do "Extract here".

## Script Execution

To send a script you can use a method like following in C#:
```csharp
private static async void SendScript(string source)
{
    string ipaddr = "127.0.0.1";
    int port = 0007; // port in the module

    try
    {
        byte[] sbytes = Encoding.UTF8.GetBytes(source);
        byte[] lbytes = BitConverter.GetBytes(sbytes.Length);

        if (BitConverter.IsLittleEndian)
            Array.Reverse(lbytes);

        using (TcpClient client = new TcpClient(ipaddr, port))
        using (NetworkStream stream = client.GetStream())
        {
            await stream.WriteAsync(lbytes, 0, lbytes.Length);
            await stream.WriteAsync(sbytes, 0, sbytes.Length);
        }
    }
    catch (Exception ex)
    {
        MessageBox.Show($"Failed to send script: {ex.Message}", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
    }
}
```

# Credits

- @kitodoescode: Main Code
- @blud_wtf: Help with TPHandler & other stuff
