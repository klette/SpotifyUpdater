#!/usr/bin/python

import json
import urllib

def fetch_list(listid, file):
    data = json.loads(urllib.urlopen('http://spotify.erlang.no/?output=json&list=%s' % listid).read())
    tmp = []
    foo = [tmp.extend(a) for a in data]
    fp = open(file, "w")
    fp.write('\n'.join([c['link'] for c in tmp if 'link' in c]))
    fp.close()

fetch_list(1, "p3.pls")
fetch_list(2, "mp3.pls")
fetch_list(3, "vg.pls")
fetch_list(4, "radio1.pls")
