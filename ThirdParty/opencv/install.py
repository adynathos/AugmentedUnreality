#!/usr/bin/env python3
import os, shutil, argparse, platform
from os.path import join as pp

modules = [
	"opencv_core",
	"opencv_aur_allocator",
	"opencv_calib3d",	# camera calibration
	"opencv_features2d", # cv::SimpleBlobDetector for calibration
	"opencv_videoio",	# VideoCapture
	"opencv_aruco",		# Aruco markers
	"opencv_imgproc",   # Aruco needs this
	"opencv_flann",     # Aruco needs this
	"opencv_imgcodecs",	# imwrite
	"opencv_video",		# Kalman filter
]

def main(args):
	print('Installing for platform:', args.platform)

	opencv_root = os.path.dirname(__file__)
	plugin_root = os.path.join(opencv_root, '..', '..')

	build_src = pp(opencv_root, 'build', args.platform)
	install_src = pp(build_src, 'install')

	binaries_dest = pp(plugin_root, 'Binaries', args.platform)

	def copy_tree(src, dest):
		print('copy dir:', src, ' -> ', dest)

		if not os.path.isdir(src):
			raise Exception('Source directory not found at '+ src)

		if os.path.exists(dest):
			print('delete existing dir:', dest)
			shutil.rmtree(dest, onerror=lambda *err: print('	rmtree err: ', err))

		print('copying dir')
		shutil.copytree(src, dest)

	def copy_single(src, dest):
		print('copy file: ', src, ' to ', dest)
		shutil.copy(src, dest)

	def copy_includes():
		print('-- Includes --')
		includes_src = pp(install_src, 'include')
		includes_dest = pp(opencv_root, 'include')
		copy_tree(includes_src, includes_dest)

	def copy_libs():
		print('-- Libs --')
		if args.platform == 'Win64':
			binaries_src = pp(install_src, 'x64', 'vc14')

			shared_lib_src = pp(binaries_src, 'bin')
			shared_lib_dest = binaries_dest
			for mod in modules:
				copy_single(pp(shared_lib_src, mod + '310.dll'), shared_lib_dest)

			static_lib_src = pp(binaries_src, 'lib')
			static_lib_dest = pp(opencv_root, 'lib', args.platform)
			for mod in modules:
				copy_single(pp(static_lib_src, mod + '310.lib'), static_lib_dest)

		elif args.platform == 'Linux':
			shared_lib_src = pp(build_src, 'lib')
			shared_lib_dest = binaries_dest
			for mod in modules:
				copy_single(pp(shared_lib_src, 'lib' + mod + '.so.3.1.0'), pp(shared_lib_dest, 'lib' + mod + '.so'))

	if args.include_files:
		copy_includes()

	if args.libs:
		copy_libs()


parser = argparse.ArgumentParser(description="Copy OpenCV compulation results from build/PLATFORM to plugin's directories")
platform_dirs = {
	'windows': 'Win64',
	'linux': 'Linux',
}
parser.add_argument('--platform', type=str, choices=platform_dirs.values(), default=platform_dirs.get(platform.system().lower(), 'Win64'))
parser.add_argument('--include_files', type=bool, default=True)
parser.add_argument('--libs', type=bool, default=True)

main(parser.parse_args())
