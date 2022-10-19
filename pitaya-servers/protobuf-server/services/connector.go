package services

import (
	"context"

	"github.com/topfreegames/libpitaya/pitaya-servers/protobuf-server/testprotos"
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

func reply(code int32, msg string) *testprotos.Response {
	return &testprotos.Response{
		Code: code,
		Msg:  msg,
	}
}

// GetSessionData gets the session data
func (c *Connector) GetSessionData(ctx context.Context) (*testprotos.SessionData, error) {
	println("Got GetSessionData MESSAGE")
	res := &testprotos.SessionData{
		Data: "THIS IS THE SESSION DATA",
	}
	return res, nil
}

// GetBigMessage returns a big message
func (c *Connector) GetBigMessage(ctx context.Context) (*testprotos.BigMessage, error) {
	println("Got GetBigMessage MESSAGE")
	msg := &testprotos.BigMessage{
		Code: "ABC-200",
		Response: &testprotos.BigMessageResponse{
			Chests: []string{
				"chchche",
				"asbcasc",
				"oijdoijqwodijqwoeijqwioj",
				"ciqwojdoqiwjdiojchchche",
				"1023doi12jdoijdoqwijoi21j",
				"1jd210",
				"c",
				"-12id-10id0-id0-iew-0ic-0i",
				"$#RIOj1j#(joiJDIOJQIWOJD))",
				"dwq0jq-cqw0921jejclsnvwejflk   qwf we  we gw eg ",
				"ijqwoidjoqijc",
				"nvweoifjoweijfoiwje910e23fj",
			},
			Npcs: map[string]*testprotos.NPC{
				"npc201": &testprotos.NPC{
					Health:   21.432,
					Name:     "this is as annoying enemy",
					PublicId: "wow, public id: 20123iq9w0e9012309812039ad",
				},
				"np20xaiwodj1": &testprotos.NPC{
					Health:   72.01,
					Name:     "ENEENENNEEMMMMMMMMMMMMMMY",
					PublicId: "randonqwdmqwoqowjdoqwjdoqjwdoqqqwoj",
				},
				"qiwjdoqijwd": &testprotos.NPC{
					Health:   99.99001,
					Name:     "QUQUQUAOAOOQOQOQOQOOQOQOQOQOQMCMCMCM",
					PublicId: "owijdoiqwjdoqwjdo-021192391231-102dii2-10-0i12d",
				},
				"banana": &testprotos.NPC{
					Health:   21.432,
					Name:     "bzbzbbabababqbbwbebqwbebqwbdiasbciuwiqub",
					PublicId: "o12e12d-120id-q0-012id0-k-0cc12 1-20d10-2kd-012ie01-i-0i12 -0id-0ic- 01k-0ie-0i12-0i-d0iasidaopjdwqijd-2",
				},
				"oqijwdoijdwoij[q12919120120991912191911111": &testprotos.NPC{
					Health:   312.001,
					Name:     "UQUQUUQ",
					PublicId: "owiqdjoiqjwdoijwoqijdi112123333333333333333333333333312oij2190d1290",
				},
			},
			Player: &testprotos.PlayerResponse{
				AccessToken: "oqijwdioj29012i3jlkad012d9j",
				Health:      87.5,
				Items: []string{
					"BigItem",
					"SmallItem",
					"InterestingWeirdItem",
					"Another",
					"One",
					"Stuff",
					"MoreStuff",
					"MoarStuff",
					"BigItem",
					"SmallItem",
					"InterestingWeirdItem",
					"Another",
				},
				Name:     "Frederico Castanha",
				PublicId: "qwid1ioj230as09d1908309asd12oij3lkjoaisjd21",
			},
		},
	}
	return msg, nil
}

// SetSessionData sets the session data
func (c *Connector) SetSessionData(ctx context.Context, data *testprotos.SessionData) (*testprotos.Response, error) {
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
func (c *Connector) NotifySessionData(ctx context.Context, data *testprotos.SessionData) {
	res := &testprotos.SessionData{
		Data: "THIS IS THE SESSION DATA",
	}

	pitaya.GetSessionFromCtx(ctx).Push("some.push.route", res)

}
