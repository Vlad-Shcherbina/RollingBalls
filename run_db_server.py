import pprint
import base64

import flask
import jinja2

import run_db
import render


app = flask.Flask(__name__)

template_loader = jinja2.FileSystemLoader('templates')
app.jinja_loader = template_loader

app.jinja_env.undefined  # because it's not documented
app.jinja_env.undefined = jinja2.StrictUndefined


@app.template_filter('data_url')
def data_url(s, mime='text/plain'):
    assert isinstance(s, str), type(s)
    return 'data:{};base64,{}'.format(mime, base64.b64encode(s.encode()).decode())


@app.route('/')
def index():
    return flask.redirect(flask.url_for('list_runs'))


@app.route('/list_runs')
def list_runs():
    return flask.render_template(
        'list_runs.html',
        runs=run_db.get_all_runs(),
        baseline_id=flask.request.args.get('baseline_id'))


@app.route('/run_details')
def run_details():
    args = flask.request.args
    id = args['id']
    run = run_db.Run(id)

    baseline_id = args.get('baseline_id')

    if baseline_id:
        baseline_run = run_db.Run(baseline_id)
        baseline_results = baseline_run.results
    else:
        baseline_results = []

    return flask.render_template(
        'run_details.html',
        id=id,
        baseline_id=baseline_id,
        table=render.render_table(run.results, baseline_results),
        debug=pprint.pformat(run.attrs))


if __name__ == '__main__':
    app.debug = True
    app.run()
