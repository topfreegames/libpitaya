package services

import (
	"context"
	//"fmt"

	"github.com/topfreegames/libpitaya/pitaya-servers/protobuf-server/protos"
	"github.com/topfreegames/pitaya"
	"github.com/topfreegames/pitaya/component"
)

// ConnectorRemote is a remote that will receive rpc's
type ConnectorRemote struct {
	component.Base
}

// Connector struct
type Connector struct {
	component.Base
}

func reply(code int32, msg string) *protos.Response {
	return &protos.Response{
		Code: code,
		Msg:  msg,
	}
}

// GetSessionData gets the session data
func (c *Connector) GetSessionData(ctx context.Context) (*protos.SessionData, error) {
	println("Got GetSessionData MESSAGE")
	res := &protos.SessionData{
		Data: "THIS IS THE SESSION DATA",
	}
	return res, nil
}

// SetSessionData sets the session data
func (c *Connector) SetSessionData(ctx context.Context, data *protos.SessionData) (*protos.Response, error) {
	println("Got SetSessionData MESSAGE")
	s := pitaya.GetSessionFromCtx(ctx)
	err := s.SetData(map[string]interface{}{
		"data": data,
	})
	if err != nil {
		return nil, pitaya.Error(err, "CN-000", map[string]string{"failed": "set data"})
	}
	return reply(200, "success"), nil
}

// NotifySessionData sets the session data
func (c *Connector) NotifySessionData(ctx context.Context, data *protos.SessionData) {
	//s := pitaya.GetSessionFromCtx(ctx)
	//err := s.SetData(data.Data)
	//if err != nil {
	//fmt.Println("got error on notify", err)
	//}
}
