# How to Use

### Connecting
```cs
client = new PitayaClient();
client.NetWorkStateChangedEvent += networkState => {
    switch(networkState) {
        case PitayaNetWorkState.Connected:
            break;
        case PitayaNetWorkState.Disconnected:
            break;
        case PitayaNetWorkState.FailToConnect:
            break;
        case PitayaNetWorkState.Kicked:
            break;
        case PitayaNetWorkState.Closed:
            break;
        case PitayaNetWorkState.Connecting:
            break;
        case PitayaNetWorkState.Timeout:
            break;
        case PitayaNetWorkState.Error:
            break;
        default:
            throw new ArgumentOutOfRangeException(nameof(networkState), networkState, null);
    }
};
client.Connect("127.0.0.1", 3241);
```

Always call `Dispose()` when you are done with the client. This is very important because unity doesn't handle unmanaged code memony very well. So calling this method will clear the memory and avoid Unity becoming unresponsive
Ex:
```cs
private void OnApplicationQuit()
{
    if(_client != null)
    {
        _client.Dispose();
        _client = null;
    }
}
```

### Making a request
```cs
client.Request("connector.getsessiondata",
    res => {
        Debug.log($"[connector.getsessiondata] - response={res}");
    },
    error => {
        Debug.log($"[connector.getsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
});
```

### Listening to a server push
```cs
_client.OnRoute("some.push.route", body => {
    Debug.log("[some.push.route] PUSH RESPONSE = " + body);
});
```

### Using Protobuf
```cs
_client.OnRoute<PushData>("some.push.route", (PushData data) => {
    Debug.log("[some.push.route] PUSH RESPONSE = " + data);
});

client.Request<RequestReponse>("connector.getsessiondata",
    (RequestReponse res) => {
        Debug.log($"[connector.getsessiondata] - response={res}");
    },
    error => {
        Debug.log($"[connector.getsessiondata] ERROR - error-code={error.Code} metadata={error.Metadata}");
});
```

### Using ssl
Copy the ca root certificate to `Assets/StreamingAssets` and initialize the client like this:
```cs
client = new PitayaClient("ca.crt");
```
