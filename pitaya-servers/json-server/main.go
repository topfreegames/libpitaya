package main

import (
	"context"
	"flag"
	"fmt"

	"strings"

	"github.com/topfreegames/libpitaya/pitaya-servers/json-server/services"
	"github.com/topfreegames/pitaya"
	"github.com/topfreegames/pitaya/acceptor"
	"github.com/topfreegames/pitaya/cluster"
	"github.com/topfreegames/pitaya/component"
	"github.com/topfreegames/pitaya/route"
	"github.com/topfreegames/pitaya/serialize/json"
)

func configureFrontend(port int) {
	ws := acceptor.NewWSAcceptor(fmt.Sprintf(":%d", port))
	tcp := acceptor.NewTCPAcceptor(fmt.Sprintf(":%d", port+1))
	tls := acceptor.NewTCPAcceptor(fmt.Sprintf(":%d", port+2),
		"./fixtures/server/pitaya.crt", "./fixtures/server/pitaya.key")

	pitaya.Register(&services.Connector{},
		component.WithName("connector"),
		component.WithNameFunc(strings.ToLower),
	)
	// pitaya.RegisterRemote(&services.ConnectorRemote{},
	// 	component.WithName("connectorremote"),
	// 	component.WithNameFunc(strings.ToLower),
	// )

	err := pitaya.AddRoute("room", func(
		ctx context.Context,
		route *route.Route,
		payload []byte,
		servers map[string]*cluster.Server,
	) (*cluster.Server, error) {
		// will return the first server
		for k := range servers {
			return servers[k], nil
		}
		return nil, nil
	})

	if err != nil {
		fmt.Printf("error adding route %s\n", err.Error())
	}

	err = pitaya.SetDictionary(map[string]uint16{
		"connector.getsessiondata": 1,
		"connector.setsessiondata": 2,
		"room.room.getsessiondata": 3,
		"onMessage":                4,
		"onMembers":                5,
		"connector.geterror":       6,
	})

	if err != nil {
		fmt.Printf("error setting route dictionary %s\n", err.Error())
	}

	pitaya.AddAcceptor(ws)
	pitaya.AddAcceptor(tcp)
	pitaya.AddAcceptor(tls)
}

func main() {
	port := flag.Int("port", 3250, "the port to listen")
	svType := flag.String("type", "connector", "the server type")

	flag.Parse()

	defer pitaya.Shutdown()

	pitaya.SetSerializer(json.NewSerializer())

	configureFrontend(*port)

	pitaya.Configure(true, *svType, pitaya.Standalone, map[string]string{})
	pitaya.Start()
}
