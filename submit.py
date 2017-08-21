#!/bin/env python

import os, sys, shutil, re

re_mm = re.compile(r'^mm\.v(?P<version>\d|[1-9]\d*)\.c$', re.I)

def get_new_version(handin_dir):
	new_version = 1
	for files in os.listdir(handin_dir):
		print files
		m = re_mm.match(files)
		if m:
			cur_version = int(m.group('version'), base=10)
			if new_version < cur_version + 1:
				new_version = cur_version + 1
	return new_version


if __name__ == "__main__":
	source = os.path.join(os.curdir, "mm.c")
	if not os.path.isfile(source):
		print >> sys.stderr, "No mm.c"
		quit(1)

	home_dir = os.getenv('HOME', os.path.join(os.sep, "home", "csapp-2017summer", os.getlogin()))
        handin_dir = os.path.join(home_dir, "handin", "malloclab")

	if not os.path.isdir(handin_dir):
		print >> sys.stderr, "No handin directory(%s)" % handin_dir
		quit(1)

	version = get_new_version(handin_dir)
	target = os.path.join(handin_dir, "mm.v%d.c" % version)
	shutil.copy2(source, target)
