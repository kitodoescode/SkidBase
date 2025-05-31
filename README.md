# SkidBase

This is a roblox exploit base, originally written by XMSW (@xmsw_ on discord) & Zsim (@zsimulators on discord), re-written by Kito (@kitodoescode on discord).

## Some Info

- 40 UNC
- loadstring
- getscriptbytecode
- Entire Debug Library (thx to @blud_wtf for "getproto" & "getprotos")

## Script Execution

To send a script you can use a method like following in c#:
```csharp
private static async void SendScript(string source)
{
    string ipaddr = "127.0.0.1"; // default ip in the module
    int port = 0007; // default port in the module

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

- Original Base: @xmsw_ & @zsimulators (discord)
- Remake: @kitodoescode (discord)
- "getproto" & "getprotos": @blud_wtf (discord)
