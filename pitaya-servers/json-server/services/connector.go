package services

import (
	"context"
	"errors"

	"github.com/topfreegames/pitaya/session"

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

// SessionData struct
type SessionData struct {
	Data map[string]interface{}
}

// Response struct
type Response struct {
	Code int32
	Msg  string
}

func reply(code int32, msg string) (*Response, error) {
	res := &Response{
		Code: code,
		Msg:  msg,
	}
	return res, nil
}

// GetHandshakeData gets the handshake data
func (c *Connector) GetHandshakeData(ctx context.Context) (*session.HandshakeData, error) {
	session := pitaya.GetSessionFromCtx(ctx)
	return session.GetHandshakeData(), nil
}

// GetSessionData gets the session data
func (c *Connector) GetSessionData(ctx context.Context) (*SessionData, error) {
	s := pitaya.GetSessionFromCtx(ctx)
	res := &SessionData{
		Data: s.GetData(),
	}
	return res, nil
}

// SetSessionData sets the session data
func (c *Connector) SetSessionData(ctx context.Context, data *SessionData) (*Response, error) {
	s := pitaya.GetSessionFromCtx(ctx)
	err := s.SetData(data.Data)
	if err != nil {
		return nil, pitaya.Error(err, "CN-000", map[string]string{"failed": "set data"})
	}
	return reply(200, "success")
}

// SendPush sends a push to a user
func (c *Connector) SendPush(ctx context.Context) (*Response, error) {
	err := pitaya.GetSessionFromCtx(ctx).Push("some.push.route", map[string]interface{}{
		"key1": 10,
		"key2": true,
	})

	if err != nil {
		return nil, err
	}

	return &Response{
		Code: 200,
		Msg:  "Ok",
	}, nil
}

func (c *Connector) GetError(ctx context.Context) (*Response, error) {
	return nil, errors.New("GetError is returning a custom error")
}
