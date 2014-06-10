from flask import Flask, request
from flask.ext.heroku import Heroku
from flask.ext.sqlalchemy import SQLAlchemy
from sqlalchemy.orm import relationship
from TwitterAPI import TwitterAPI
import logging

log = logging.getLogger(__name__)

app = Flask(__name__)
heroku = Heroku(app)
db = SQLAlchemy(app)
MyScreenName = 'VoltaCoffee'
screen_names = {}


### Models

class TwitterApp(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    key = db.Column(db.String(256))
    secret = db.Column(db.String(256))
    tokens = relationship("TwitterToken", backref="app")

    def __repr__(self):
        return '<App %r>' % self.key


class TwitterToken(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    app_id = db.Column(db.Integer, db.ForeignKey('twitter_app.id'))
    token = db.Column(db.String(256))
    token_secret = db.Column(db.String(256))

    def __repr__(self):
        return '<Token %r for %r>' % (self.token, self.app)


### Routes

@app.route('/')
def home():
    return '<h1>Nothing to see here, move along</h1>'


@app.route('/twitter/<int:tid>/<path:path>')
def twitter_call(tid, path):
    api = get_api(tid)
    r = api.request(path, request.args)
    return r.text, r.status_code, dict(r.headers)


@app.route('/arduino/<int:tid>/command')
def get_command(tid):
    api = get_api(tid)
    r = api.request('statuses/mentions_timeline')
    command = ''
    for m in r.get_iterator():
        log.error('%s %s', m['text'], m['user'])
        if m['user']['following']:
            break
    # we got a command, see if we responded to it yet
    r = api.request(
        'statuses/user_timeline',
        {'screen_name': MyScreenName, 'count': 50}
    )
    command = ''
    for my_m in r.get_iterator():
        if my_m['in_reply_to_status_id'] == m['id_str']:
            # we responded to this recently
            break
        if 'awaiting orders' in my_m['text'] and (
            int(my_m['id_str']) > int(m['id_str'])
        ):
            # we just started up, no need to read more
            break
    else:
        t = m['text'].lower()
        if ' status' in t:
            command = 'status'
        elif ' start' in t:
            command = 'start'
        elif ' stop' in t:
            command = 'stop'
            api.request('statuses/update', {
                'in_reply_to_status_id_str': m['id_str'],
                'status': "@%s couldn't understand that, try again" % (
                    m['user']['screen_name']
                ),
            })
        if command:
            command += " %(id_str)s" % m
            screen_names[m['id_str']] = m['user']['screen_name']
    return '*'+command, 200


@app.route('/arduino/<int:tid>/respond')
def respond_command(tid):
    api = get_api(tid)
    mid = request.args['mid']
    command = request.args['command']
    screen_name = screen_names[mid]
    c, s = command.split()
    log.error('%s %s %s', c, s, mid)
    if c == 'start':
        if s == '1':
            msg = 'Starting the coffee machine, '
        else:
            msg = 'coffee machine already on, not doing anything'
    elif c == 'stop':
        if s == '1':
            msg = 'Stopped the coffee machine'
        else:
            msg = 'coffee machine is already off, not doing anything'
    elif c == 'status':
        if s == '1':
            msg = 'Coffee machine is running!'
        else:
            msg = 'coffee machine is off'
    else:
        msg = 'unkown???'
    r = api.request('statuses/update', {
        'in_reply_to_status_id_str': mid,
        'status': "@%s %s <-- %s" % (
            screen_name,
            msg,
            mid,
        ),
    })
    log.error(r.text)
    return '', 200


## local helpers
def get_api(tid):
    token = TwitterToken.query.filter_by(id=tid).first()
    api = TwitterAPI(
        token.app.key, token.app.secret,
        token.token, token.token_secret,
    )
    return api
