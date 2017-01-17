#!/usr/bin/env python

import os
import os.path as path


def createDrawableStateList(name):
	if path.isfile("AndroidManifest.xml"):
		PROJECT_DIR = os.getcwd()
	elif path.isfile("../AndroidManifest.xml"):
		PROJECT_DIR = path.dirname(os.getcwd())
	
	filename = PROJECT_DIR + "/res/drawable/" + name + ".xml"
	fd = open(filename, "wt")
	try:
		fd.write('<?xml version="1.0" encoding="utf-8"?>\n')
		fd.write('<selector xmlns:android="http://schemas.android.com/apk/res/android">\n')
		fd.write('\t<item android:drawable="@drawable/' + name + '_pressed" android:state_pressed="true" />\n')
		fd.write('\t<item android:drawable="@drawable/' + name + '_pressed" android:state_selected="true" />\n')
		fd.write('\t<item android:drawable="@drawable/' + name + '_normal" />\n')
		fd.write('</selector>')
	finally:
		fd.close()
	
if __name__ == '__main__':
#	createDrawableStateList('ratio_free')
	ratios = ['ratio_free', 'ratio_1_1', 'ratio_2_3', 'ratio_3_2', 'ratio_3_4', 'ratio_4_3', 'ratio_9_16', 'ratio_16_9']
	for ratio in ratios:
		createDrawableStateList(ratio)
	
