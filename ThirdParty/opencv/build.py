#!/usr/bin/env python3
import click, subprocess, os
import shutil, glob
from os.path import join as pp
from platform import system as system_name
from colorama import init
from colorama import Fore as color
from colorama import Back as color_background
init(autoreset=True)

# Dirs
dir_opencv_root = os.path.dirname(os.path.realpath(__file__))
dir_src = pp(dir_opencv_root, 'src', 'opencv')
dir_cmake = pp(dir_opencv_root, 'cmake_settings')
dir_plugin_root = pp(dir_opencv_root, '..', '..')

# Platform
platform_dirs = {
	'windows': 'Win64',
	'linux': 'Linux',
	'android': 'Android',
}
platforms = list(platform_dirs.keys())
platforms.sort()

platform_current = system_name().lower()
if platform_current not in platform_dirs:
	print('Warning: unrecognized platform: ', platform_current)
	platform_current = platforms[0]

# OpenCV Modules
MODULES = [
	"opencv_core",
	"opencv_augmented_unreality", # parts of our plugin that are easier to build as a custom OpenCV module
	"opencv_aruco",		# Aruco markers
	"opencv_calib3d",	# camera calibration
	"opencv_features2d", # cv::SimpleBlobDetector for calibration
	"opencv_flann",     # Aruco needs this
	"opencv_imgcodecs",	# imwrite
	"opencv_imgproc",   # Aruco needs this
	"opencv_video",		# Kalman filter
	"opencv_videoio",	# VideoCapture
]


@click.group()
def main():
	pass

@main.command(help = """
Runs CMake to generate the build files which can be then used to build the OpenCV in a platform-specific way.
Procedure:
- create a directory for OpenCV build for this platform
- run: python build.py build PLATFORM --build_dir MY_BUILD_DIR
- follow the instruction printed by the program
- run: python build.py copy PLATFORM
""")
@click.argument('platform', type=click.Choice(platforms), default=platform_current)
@click.option('--build_dir', type=click.Path(file_okay=False, resolve_path=True), default='.')
def build(platform, build_dir):
	dir_install = pp(dir_opencv_root, 'install', platform_dirs[platform])

	print('Task:', color.MAGENTA + 'build')
	print('- platform:', color.CYAN + platform)
	print('- build directory:', color.CYAN + build_dir)
	print('- install directory:', color.CYAN + dir_install)

	# create build dir if it does not exist
	os.makedirs(build_dir, exist_ok=True)

	# move to that directory as cmake likes to be executed in the build dir
	os.chdir(build_dir)

	print('Running ', color.GREEN + 'CMake')

	if platform == 'windows':
		print('To play video files, install ', color.CYAN + 'GStreamer', ' (and its development files)')
		print('and set env variable ', color.RED + 'GSTREAMER_DIR', ' to ', color.YELLOW + '(gstreamer_install_dir)/1.0/x86_64')
	
	print(color_background.GREEN + 80*' ')

	# run cmake
	cmd_cmake = [
		'cmake'
	]

	if platform == 'windows':
		cmd_cmake += [
			'-G', 'Visual Studio 15 2017 Win64'
		]

	cmd_cmake += [	
		'-C', pp(dir_cmake, platform + '.cmake'), # load predefined cache
		'-DCMAKE_INSTALL_PREFIX={d}'.format(d=dir_install),
		dir_src # source dir
	]

	# twice because sometimes there are error at first time
	out_code = subprocess.call(cmd_cmake)

	if out_code:
		print('Retrying')
		out_code = subprocess.call(cmd_cmake)

	if out_code:
		print(color_background.RED + 80*' ')
		print(color.RED + 'CMake returned an error, the build files may be wrong')

	else:
		print(color_background.GREEN + 80*' ')
		print('Build files have been generated')
		print('To continue build, go to ', color.YELLOW + build_dir + color.RESET, ' and:')

		if platform in {'linux', 'android'}:
			print('	run command ', color.GREEN + 'make -j8 install')
		elif platform == 'windows':
			print('	build the project with ', color.GREEN + 'Visual Studio')

@main.command(help="""
Copies the OpenCV binaries (after build) to their correct location.
""")
@click.argument('platform', type=click.Choice(platforms), default=platform_current)
@click.option('--copy_includes', type=bool, default=True, help="Copy the header files?")
@click.option('--copy_binaries', type=bool, default=True, help="Copy the compiled modules?")
def copy(platform, copy_includes, copy_binaries):
	# where the "make install" wrote its files:
	dir_install = pp(dir_opencv_root, 'install', platform_dirs[platform])

	# where we want to copy the files
	dir_binaries_dest = pp(dir_plugin_root, 'Binaries', platform_dirs[platform])

	print('Task:', color.MAGENTA + 'build')
	print('- platform:', color.CYAN + platform)
	print('- source directory:', color.CYAN + dir_install)
	print('- destination directory:', color.CYAN + dir_binaries_dest)

	def copy_tree(src, dest):
		print('copy directory: ', color.YELLOW + src + color.RESET, ' ---> ', color.CYAN + dest)

		if not os.path.isdir(src):
			raise Exception('Source directory not found at '+ src)

		if os.path.exists(dest):
			print('	delete existing dir:', dest)
			shutil.rmtree(dest, onerror=lambda *err: print('	rmtree err: ', err))

		print('	copying dir')
		shutil.copytree(src, dest)

	def copy_single(src, dest):
		print('copy : ', color.YELLOW + src + color.RESET, ' ---> ', color.CYAN + dest)
		shutil.copy(src, dest)

	def func_copy_includes():
		print('-- Includes --')

		if platform != 'android':
			includes_src = pp(dir_install, 'include')
		else:
			includes_src = pp(dir_install, 'sdk', 'native', 'jni', 'include')

		includes_dest = pp(dir_opencv_root, 'include')
		copy_tree(includes_src, includes_dest)

	def copy_libs():
		print('-- Libs --')
		modules = MODULES

		if platform == 'windows':
			#modules = MODULES + ["opencv_aur_allocator"]

			binaries_src = pp(dir_install, 'x64', 'vc15')

			shared_lib_src = pp(binaries_src, 'bin')
			for mod in modules:
				copy_single(pp(shared_lib_src, mod + '320.dll'), dir_binaries_dest)

			# static_lib_src = pp(binaries_src, 'lib')
			# static_lib_dest = pp(opencv_root, 'lib', args.platform)
			# for mod in modules:
			# 	copy_single(pp(static_lib_src, mod + '310.lib'), static_lib_dest)

		elif platform == 'linux':
			shared_lib_src = pp(dir_install, 'lib')
			for mod in modules:
				symlink_name = pp(dir_binaries_dest, 'lib' + mod + '.so')
				out_name = symlink_name + '.3.2'

				copy_single(pp(shared_lib_src, 'lib' + mod + '.so.3.2.0'), out_name)

				try:
					os.remove(symlink_name)
				except:
					pass
				os.symlink(out_name, symlink_name)

		elif platform == 'android':
			print(color.GREEN + 'Android libraries are statically linked so no need to move them')

	if copy_includes:
		func_copy_includes()

	if copy_binaries:
		copy_libs()

if __name__ == '__main__':
	main()
