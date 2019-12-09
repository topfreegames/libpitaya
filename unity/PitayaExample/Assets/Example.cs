using System.Collections.Generic;
using System.IO;
using UnityEngine;
using Pitaya;
using Pitaya.SimpleJson;
using UnityEngine.UI;

public class Example : MonoBehaviour
{
	private PitayaClient _client;
	private bool _connected;
	private bool _requestSent;

	public Button GetDataButton;

	// Use this for initialization
	private void Start()
	{
		GetDataButton.onClick.AddListener(() =>
		{
			_client.Request("connector.getsessiondata",
				action: data =>
				{
					Debug.LogFormat("GetSessionData: {0}", data);
				},
				errorAction: err =>
				{
					Debug.LogFormat("GetSessionData Error: {0}", err);
				});
		});
		
		// _client = new PitayaClient("ca.crt");
		_client = new PitayaClient(metricsCb: stats =>
		{
			Debug.Log("=========> Received connection stats!");
			Debug.Log(stats.Serialize());
		});
		_connected = false;
		_requestSent = false;

		_client.NetWorkStateChangedEvent += (ev, error) =>
		{
			if (ev == PitayaNetWorkState.Connected)
			{
				Debug.Log("Successfully connected!");
				_connected = true;
			}
			else if (ev == PitayaNetWorkState.FailToConnect)
			{
				Debug.Log("Failed to connect");
			}
		};

		_client.Connect("libpitaya-tests.tfgco.com", 3251,
			new Dictionary<string, string>
            {
                {"oi", "mano"}
            });
	}

	// Update is called once per frame
	private void Update()
	{
		if (_connected && !_requestSent)
		{
			_client.Request("connector.getsessiondata",
				action: data =>
                {
					Debug.Log("Got request data: " + data);
                },
				errorAction: (err) =>
				{
					Debug.LogError("Got error: code = " + err.Code + ", msg = " + err.Msg);
				});
			_requestSent = true;
		}
	}

	private void OnDestroy()
	{
		if (_client != null)
		{
			_client.Dispose();
			_client = null;
		}
	}
}
