package services

import (
	"context"
	"fmt"
	"time"

	"github.com/topfreegames/pitaya/session"

	opentracing "github.com/opentracing/opentracing-go"
	"github.com/topfreegames/pitaya"
	"github.com/topfreegames/pitaya/component"
	"github.com/topfreegames/pitaya/tracing/jaeger"
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
	println("GETTING HANDSHAKE DATA")
	session := pitaya.GetSessionFromCtx(ctx)
	return session.GetHandshakeData(), nil
}

// GetSessionData gets the session data
func (c *Connector) GetSessionData(ctx context.Context) (*SessionData, error) {
	parentSpanCtx, err := pitaya.ExtractSpan(ctx)
	if err != nil {
		return nil, pitaya.Error(err, "RH-001", map[string]string{"failed": "span from context"})
	}
	tags1 := opentracing.Tags{
		"span.kind":    "client",
		"db.type":      "sql",
		"db.name":      "postgres",
		"db.statement": "SELECT * FROM users;",
	}
	reference1 := opentracing.ChildOf(parentSpanCtx)
	span1 := opentracing.StartSpan("SQL SELECT", reference1, tags1)
	time.Sleep(20 * time.Millisecond)
	span1.Finish()

	tags2 := opentracing.Tags{
		"span.kind":    "client",
		"db.name":      "redis",
		"db.statement": "KEYS *",
	}
	reference2 := opentracing.ChildOf(parentSpanCtx)
	span2 := opentracing.StartSpan("REDIS KEYS", reference2, tags2)
	time.Sleep(40 * time.Millisecond)
	span2.Finish()

	tags3 := opentracing.Tags{
		"span.kind": "client",
	}
	reference3 := opentracing.ChildOf(parentSpanCtx)
	span3 := opentracing.StartSpan("HTTP PUT podium.podium.svc.cluster.local.:80", reference3, tags3)
	time.Sleep(10 * time.Millisecond)
	tags4 := opentracing.Tags{
		"span.kind": "server",
	}

	_, err = jaeger.Configure(jaeger.Options{
		Disabled:    false,
		Probability: 1.0,
		ServiceName: "podium",
	})
	if err != nil {
		fmt.Println("failed to configure jaeger")
	}
	reference4 := opentracing.ChildOf(span3.Context())
	span4 := opentracing.StartSpan("HTTP PUT /m/:memberPublicID/scores", reference4, tags4)
	time.Sleep(100 * time.Millisecond)
	span4.Finish()
	_, err = jaeger.Configure(jaeger.Options{
		Disabled:    false,
		Probability: 1.0,
		ServiceName: "connector",
	})
	if err != nil {
		fmt.Println("failed to configure jaeger")
	}

	time.Sleep(10 * time.Millisecond)
	span3.Finish()

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
