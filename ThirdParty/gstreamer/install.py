#!/usr/bin/env python3
import os, shutil, argparse, platform
from os.path import join as pp

modules = set(lb.lower() for lb in [
	"libffi-7.dll",
	"libglib-2.0-0.dll",
	"libgmodule-2.0-0.dll",
	"libgobject-2.0-0.dll",
	"libgstapp-1.0-0.dll",
	"libgstaudio-1.0-0.dll",
	"libgstbase-1.0-0.dll",
	"libgstpbutils-1.0-0.dll",
	"libgstreamer-1.0-0.dll",
	"libgstriff-1.0-0.dll",
	"libgsttag-1.0-0.dll",
	"libgstvideo-1.0-0.dll",
	"libintl-8.dll",
	"liborc-0.4-0.dll",
	"libwinpthread-1.dll",
	"libz.dll",
])

# modules = set(lb.lower() for lb in [
# 	"libbz2.dll",
# 	"libcairo-2.dll",
# 	"libcairo-gobject-2.dll",
# 	"libcroco-0.6-3.dll",
# 	"libdv-4.dll",
# 	"libexpat-1.dll",
# 	"libFLAC-8.dll",
# 	"libfontconfig-1.dll",
# 	"libfreetype-6.dll",
# 	"libgcc_s_sjlj-1.dll",
# 	"libgdk_pixbuf-2.0-0.dll",
# 	"libgio-2.0-0.dll",
# 	"libgraphene-1.0-0.dll",
# 	"libgstadaptivedemux-1.0-0.dll",
# 	"libgstbadaudio-1.0-0.dll",
# 	"libgstbadbase-1.0-0.dll",
# 	"libgstbadvideo-1.0-0.dll",
# 	"libgstcodecparsers-1.0-0.dll",
# 	"libgstfft-1.0-0.dll",
# 	"libgstgl-1.0-0.dll",
# 	"libgstnet-1.0-0.dll",
# 	"libgstrtp-1.0-0.dll",
# 	"libgstrtsp-1.0-0.dll",
# 	"libgstrtspserver-1.0-0.dll",
# 	"libgstsdp-1.0-0.dll",
# 	"libgsturidownloader-1.0-0.dll",
# 	"libharfbuzz-0.dll",
# 	"libiconv-2.dll",
# 	"libjpeg-8.dll",
# 	"libkate-1.dll",
# 	"libnettle-6-2.dll",
# 	"libnice-10.dll",
# 	"libogg-0.dll",
# 	"libopenh264.dll",
# 	"libopenjpeg-1.dll",
# 	"libopus-0.dll",
# 	"libpango-1.0-0.dll",
# 	"libpangocairo-1.0-0.dll",
# 	"libpangoft2-1.0-0.dll",
# 	"libpangowin32-1.0-0.dll",
# 	"libpixman-1-0.dll",
# 	"libpng16-16.dll",
# 	"librsvg-2-2.dll",
# 	"libsbc-1.dll",
# 	"libschroedinger-1.0-0.dll",
# 	"libSoundTouch-0.dll",
# 	"libsoup-2.4-1.dll",
# 	"libspandsp-2.dll",
# 	"libspeex-1.dll",
# 	"libsrtp.dll",
# 	"libstdc++-6.dll",
# 	"libtag.dll",
# 	"libtheoradec-1.dll",
# 	"libtheoraenc-1.dll",
# 	"libtiff-5.dll",
# 	"libvisual-0.4-0.dll",
# 	"libvorbis-0.dll",
# 	"libvorbisenc-2.dll",
# 	"libwavpack-1.dll",
# 	"libwebrtc_audio_processing-1.dll",
# 	"libxml2-2.dll",
# ])

def main(args):
	print('Platform:', args.platform)
	print('GStreamer:', args.gstreamer_root)

	plugin_root = os.path.join(os.path.dirname(__file__), '..', '..')
	binaries_dest = pp(plugin_root, 'Binaries', args.platform)

	def copy_single(src, dest):
		print('copy file: ', src, ' to ', dest)
		shutil.copy(src, dest)

	def copy_libs():
		print('-- Libs --')
		if args.platform == 'Win64':
			binaries_src = pp(args.gstreamer_root)

			shared_lib_src = pp(binaries_src, 'bin')
			shared_lib_dest = binaries_dest

			for lib_file in os.listdir(shared_lib_src):
				#if os.path.splitext(lib_file)[1].lower() == '.dll':
				if lib_file.lower() in  modules:
					copy_single(pp(shared_lib_src, lib_file), shared_lib_dest)

		elif args.platform == 'Linux':
			print('No linux binaries so far')

	if args.libs:
		copy_libs()


parser = argparse.ArgumentParser(description="Copy OpenCV compulation results from build/PLATFORM to plugin's directories")

platform_dirs = {
	'windows': 'Win64',
	'linux': 'Linux',
}
parser.add_argument('--platform', type=str, choices=platform_dirs.values(), default=platform_dirs.get(platform.system().lower(), 'Win64'))
parser.add_argument('--libs', type=bool, default=True)
parser.add_argument('--gstreamer_root', type=str, default=os.environ.get('GSTREAMER_1_0_ROOT_X86_64', '.'))

main(parser.parse_args())
