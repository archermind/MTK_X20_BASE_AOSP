#!/usr/bin/python
#
# Copyright (c) 2015 MediaTek Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import argparse
import json
import os
import sys

PROJECT="k35mv1_64_trusty"
PLATFORM="mt6735"
ARCH_64=True
KERNEL=""
FEATURE_OPTIONS_JSON=""
ANDROID_BUILD_TOP="./../../../../../../../.."

HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'
BOLD = "\033[1m"

def infog(msg):
    print OKGREEN + msg + ENDC

def info(msg):
    print OKBLUE + msg + ENDC

def warn(msg):
    print WARNING + msg + ENDC

def err(msg):
    print FAIL + msg + ENDC

def dump_options(config, options):
	print config
	opts_not_found = dict(options)
	with open(config, 'r') as f:
		for line in f:
			line = line.strip()
			for opt in options:
				if line.startswith(opt):
					actual = line[line.index('=')+1:].strip()
					expected = options[opt]
					if actual == expected:
						info("\t%s" % line.strip())
					else:
						infog("\t%s, EXPECTED(%s)" % (line.strip(), expected))
					del opts_not_found[opt]

	if opts_not_found:
		for opt in opts_not_found:
			warn("\t%s not set" % opt)

def main():
	with open(FEATURE_OPTIONS_JSON, 'r') as f:
		fos = json.load(f)

	fos["kernel"]["CONFIG"] = fos["kernel"]["CONFIG"].replace("${KERNEL}", KERNEL)
	fos["kernel-debug"]["CONFIG"] = fos["kernel-debug"]["CONFIG"].replace("${KERNEL}", KERNEL)

	if ARCH_64:
		fos["kernel"]["CONFIG"] = fos["kernel"]["CONFIG"].replace("${ARCH}", "arm64")
		fos["kernel-debug"]["CONFIG"] = fos["kernel-debug"]["CONFIG"].replace("${ARCH}", "arm64")
	else:
		fos["kernel"]["CONFIG"] = fos["kernel"]["CONFIG"].replace("${ARCH}", "arm")
		fos["kernel-debug"]["CONFIG"] = fos["kernel-debug"]["CONFIG"].replace("${ARCH}", "arm")

	for m in sorted(fos):
		fos[m]["CONFIG"] = fos[m]["CONFIG"].replace("${PROJECT}", PROJECT)
		fos[m]["CONFIG"] = fos[m]["CONFIG"].replace("${PLATFORM}", PLATFORM)
		fos[m]["CONFIG"] = ANDROID_BUILD_TOP + '/' + fos[m]["CONFIG"]
		if os.path.isfile(fos[m]["CONFIG"]):
			dump_options(fos[m]["CONFIG"], fos[m]["OPTIONS"])
		else:
			err("%s not exist, ignore!" % fos[m]["CONFIG"])

if __name__ == "__main__":

#	ANDROID_BUILD_TOP = os.getenv('ANDROID_BUILD_TOP')

#	if not ANDROID_BUILD_TOP:
#		print "\"ANDROID_BUILD_TOP\" NOT set! Please \"source build/envsetup.sh\" & \"lunch\""
#		sys.exit(1)

	parser = argparse.ArgumentParser()
	parser.add_argument("--project", help="MTK Project Name")
	parser.add_argument("--platform", help="MTK Platform Name")
	parser.add_argument("--kernel", help="MTK Project Kernel Version")
	parser.add_argument("feature_options_json", help="MTK Feature Options JSON")
	args = parser.parse_args()

	if args.project and args.platform and args.kernel:
		PROJECT=args.project
		ARCH_64 = True if PROJECT.find("64") != -1 else False
		PLATFORM=args.platform
		KERNEL=args.kernel
		FEATURE_OPTIONS_JSON = args.feature_options_json
		print "PROJECT: %s" % PROJECT
		print "ARCH_64: %s" % ARCH_64
		print "PLATFORM: %s" % PLATFORM
		print "KERNEL: %s" % KERNEL
		main()
	else:
		print "PROJECT or PLATFORM or KERNEL is empty."
		print "Usage: %s -h or --help." % sys.argv[0]
