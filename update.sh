# Needs seperate compiles updaters for now..

python -c "import lxml.html; import urllib; print '\n'.join([a.values()[0] for a in lxml.html.document_fromstring(urllib.urlopen('http://spotify.erlang.no').read()).cssselect('#p3')[0].cssselect('a')])" > p3.pls
python -c "import lxml.html; import urllib; print '\n'.join([a.values()[0] for a in lxml.html.document_fromstring(urllib.urlopen('http://spotify.erlang.no').read()).cssselect('#mpetre')[0].cssselect('a')])" > mp3.pls
python -c "import lxml.html; import urllib; print '\n'.join([a.values()[0] for a in lxml.html.document_fromstring(urllib.urlopen('http://spotify.erlang.no').read()).cssselect('#vglista')[0].cssselect('a')])" > vg.pls
python -c "import lxml.html; import urllib; print '\n'.join([a.values()[0] for a in lxml.html.document_fromstring(urllib.urlopen('http://spotify.erlang.no').read()).cssselect('#radio1')[0].cssselect('a')])" > radio1.pls

./updater-p3 username pass
./updater-mp3 username pass
./updater-vg username pass
./updater-radio1 username pass