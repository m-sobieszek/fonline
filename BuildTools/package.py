#!/usr/bin/python3

import os
import argparse
import sys
import shutil
import zipfile
import subprocess
import tarfile
import glob

parser = argparse.ArgumentParser(description='FOnline packager')
parser.add_argument('-buildhash', dest='buildhash', required=True, help='build hash')
parser.add_argument('-devname', dest='devname', required=True, help='Dev game name')
parser.add_argument('-nicename', dest='nicename', required=True, help='Representable game name')
parser.add_argument('-authorname', dest='authorname', required=True, help='Author name')
parser.add_argument('-gameversion', dest='gameversion', required=True, help='Game version')
parser.add_argument('-target', dest='target', required=True, choices=['Server', 'Client', 'Single', 'Mapper'], help='package target type')
parser.add_argument('-platform', dest='platform', required=True, choices=['Windows', 'Linux', 'Android', 'macOS', 'iOS', 'Web'], help='platform type')
parser.add_argument('-arch', dest='arch', required=True, help='architectures to include (divided by +)')
# Windows: win32 win64
# Linux: x64
# Android: arm arm64 x86
# macOS: x64
# iOS: arm64
# Web: wasm
parser.add_argument('-pack', dest='pack', required=True, help='package type')
# Windows: Raw Zip Wix
# Linux: Raw Tar Zip AppImage
# Android: Raw Apk
# macOS: Raw Bundle
# iOS: Raw Bundle
# Web: Raw
parser.add_argument('-debug', dest='debug', action='store_true', help='debug mode')
parser.add_argument('-configname', dest='configname', required=True, help='config name')
parser.add_argument('-config', dest='config', required=True, action='append', default=[], help='config tweaks')
parser.add_argument('-angelscript', dest='angelscript', action='store_true', help='attach angelscript scripts')
parser.add_argument('-mono', dest='mono', action='store_true', help='attach mono scripts')
parser.add_argument('-input', dest='input', required=True, action='append', default=[], help='input dir (from FONLINE_OUTPUT_PATH)')
parser.add_argument('-output', dest='output', required=True, help='output dir')
args = parser.parse_args()

print('[Package]', f'Make {args.configname} {args.target} for {args.platform} to {args.output}')

outputPath = (args.output if args.output else os.getcwd()).rstrip('\\/')
buildToolsPath = os.path.dirname(os.path.realpath(__file__))

curPath = os.path.dirname(sys.argv[0])
gameName = sys.argv[1]

#os.environ['JAVA_HOME'] = sys.argv[6]
#os.environ['ANDROID_HOME'] = sys.argv[7]
#os.environ['EMSCRIPTEN'] = sys.argv[8]

# Generate config
config = {}
configStr = ''
for cfg in args.config:
	k, v = cfg.split(',', 1)
	#print(k, v)
	configStr += k + '=' + v + '\n'

# Find files
def getInput(subdir, inputType):
	for i in args.input:
		absDir = os.path.abspath(i)
		absDir = os.path.join(absDir, subdir)
		if os.path.isdir(absDir):
			buildHashPath = os.path.join(absDir, inputType + '.build-hash')
			if os.path.isfile(buildHashPath):
				with open(buildHashPath, 'r', encoding='utf-8-sig') as f:
					buildHash = f.read()
				
				if buildHash == args.buildhash:
					return absDir
				
				else:
					assert False, 'Build hash file ' + buildHashPath + ' has wrong hash'
			else:
				assert False, 'Build hash file ' + buildHashPath + ' not found'
	else:
		assert False, 'Input dir ' + subdir + ' not found for ' + inputType

def getLogo():
	logoPath = os.path.join(outputPath, 'Logo.png')
	if not os.path.isfile(logoPath):
		logoPath = os.path.join(binariesPath, 'DefaultLogo.png')
	import PIL
	return PIL.Image.open(logoPath)

def patchConfig(filePath):
	with open(filePath, 'rb') as f:
		file = f.read()
	fileSize = os.path.getsize(filePath)
	pos1 = file.find(b'###InternalConfig###')
	assert pos1 != -1
	pos2 = file.find(b'\0', pos1)
	assert pos2 != -1
	pos3 = file.find(b'\0', pos2 + 1)
	assert pos3 != -1
	size = pos3 - pos1
	assert size + 1 == 5022 # Magic
	configBytes = str.encode(configStr)
	assert len(configBytes) <= size
	padding = b'#' * (size - len(configBytes))
	file = file[0:pos1] + configBytes + padding + file[pos3:]
	with open(filePath, 'wb') as f:
		f.write(file)
	assert fileSize == os.path.getsize(filePath)

def patchFile(filePath, textFrom, textTo):
	with open(filePath, 'rb') as f:
		content = f.read()
	content = content.replace(textFrom, textTo)
	with open(filePath, 'wb') as f:
		f.write(content)

def makeZip(name, path):
	zip = zipfile.ZipFile(name, 'w', zipfile.ZIP_DEFLATED)
	for root, dirs, files in os.walk(path):
		for file in files:
			zip.write(os.path.join(root, file), os.path.join(os.path.relpath(root, path), file))
	zip.close()

def makeTar(name, path, mode):
	def filter(tarinfo):
		#tarinfo.mode = 0777
		return tarinfo
	tar = tarfile.open(name, mode)
	dir = os.getcwd()
	newDir, folder = os.path.split(path)
	os.chdir(newDir)
	tar.add(folder, filter = filter)
	os.chdir(dir)
	tar.close()

def build():
	if args.platform == 'Windows':
		# Raw files
		os.makedirs(os.path.join(targetOutputPath, 'Resources'))

		resourcesPath = getInput('Resources', 'Resources')
		for f in glob.glob(os.path.join(resourcesPath, '*.zip')):
			shutil.copy(os.path.join(resourcesPath, f), os.path.join(targetOutputPath, 'Resources'))
		for f in glob.glob(os.path.join(resourcesPath, 'Raw', '*')):
			shutil.copy(os.path.join(resourcesPath, f), os.path.join(targetOutputPath, 'Resources'))

		for arch in args.arch.split('+'):
			binPath = getInput(os.path.join('Binaries', args.target + '-' + args.platform + '-' + arch + ('-Debug' if args.debug else '')), 'FOnline' + args.target)
			binAppendix = '_' + args.target if args.target != 'Client' and args.target != 'Single' else ''
			shutil.copy(os.path.join(binPath, 'FOnline' + args.target + '.exe'), os.path.join(targetOutputPath, args.devname + binAppendix + '.exe'))
			shutil.copy(os.path.join(binPath, 'FOnline' + args.target + '.pdb'), os.path.join(targetOutputPath, args.devname + binAppendix + '.pdb'))
			patchConfig(os.path.join(targetOutputPath, args.devname + binAppendix + '.exe'))

		if args.angelscript:
			scriptsPath = getInput('Scripts', 'AngelScript')
			shutil.copy(os.path.join(scriptsPath, args.target + 'RootModule.fosb'), os.path.join(targetOutputPath, 'Resources'))
			if args.target == 'Server':
				shutil.copy(os.path.join(scriptsPath, 'ClientRootModule.fosb'), os.path.join(targetOutputPath, 'Resources'))

		# Zip
		"""
		if 'Zip' in args.pack:
			makeZip(targetOutputPath + '/' + gameName + '.zip', targetOutputPath)

		# MSI Installer
		sys.path.insert(0, os.path.join(curPath, 'msicreator'))
		import createmsi
		import uuid

		msiConfig = "" " \
		{
			"upgrade_guid": "%s",
			"version": "%s",
			"product_name": "%s",
			"manufacturer": "%s",
			"name": "%s",
			"name_base": "%s",
			"comments": "%s",
			"installdir": "%s",
			"license_file": "%s",
			"need_msvcrt": %s,
			"arch": %s,
			"parts":
			[ {
					"id": "%s",
					"title": "%s",
					"description": "%s",
					"absent": "%s",
					"staged_dir": "%s"
			} ]
		}"" " % (uuid.uuid3(uuid.NAMESPACE_OID, gameName), '1.0.0', \
				gameName, 'Dream', gameName, gameName, 'The game', \
				gameName, 'License.rtf', 'false', 32, \
				gameName, gameName, 'MMORPG', 'disallow', gameName)

		try:
			cwd = os.getcwd()
			wixBinPath = os.path.abspath(os.path.join(curPath, 'wix'))
			wixTempPath = os.path.abspath(os.path.join(targetOutputPath, 'WixTemp'))
			os.environ['WIX_TEMP'] = wixTempPath
			os.makedirs(wixTempPath)

			licensePath = os.path.join(outputPath, 'MsiLicense.rtf')
			if not os.path.isfile(licensePath):
				licensePath = os.path.join(binariesPath, 'DefaultMsiLicense.rtf')
			shutil.copy(licensePath, os.path.join(targetOutputPath, 'License.rtf'))

			with open(os.path.join(targetOutputPath, 'MSI.json'), 'wt') as f:
				f.write(msiConfig)

			os.chdir(targetOutputPath)

			msi = createmsi.PackageGenerator('MSI.json')
			msi.generate_files()
			msi.final_output = gameName + '.msi'
			msi.args1 = ['-nologo', '-sw']
			msi.args2 = ['-sf', '-spdb', '-sw', '-nologo']
			msi.build_package(wixBinPath)

			shutil.rmtree(wixTempPath, True)
			os.remove('MSI.json')
			os.remove('License.rtf')
			os.remove(gameName + '.wixobj')
			os.remove(gameName + '.wxs')

		except Exception as e:
			print(str(e))
		finally:
			os.chdir(cwd)

		# Update binaries
		binPath = resourcesPath + '/../Binaries'
		if not os.path.exists(binPath):
			os.makedirs(binPath)
		shutil.copy(gameOutputPath + '/' + gameName + '.exe', binPath + '/' + gameName + '.exe')
		shutil.copy(gameOutputPath + '/' + gameName + '.pdb', binPath + '/' + gameName + '.pdb')
		"""

	elif args.platform == 'Linux':
		# Raw files
		os.makedirs(gameOutputPath)
		shutil.copytree(resourcesPath, gameOutputPath + '/Data')
		# shutil.copy(binariesPath + '/Linux/FOnline32', gameOutputPath + '/' + gameName + '32')
		shutil.copy(binariesPath + '/Linux/FOnline64', gameOutputPath + '/' + gameName + '64')
		# patchConfig(gameOutputPath + '/' + gameName + '32')
		patchConfig(gameOutputPath + '/' + gameName + '64')

		# Tar
		makeTar(targetOutputPath + '/' + gameName + '.tar', gameOutputPath, 'w')
		makeTar(targetOutputPath + '/' + gameName + '.tar.gz', gameOutputPath, 'w:gz')

	elif args.platform == 'Mac':
		# Raw files
		os.makedirs(gameOutputPath)
		shutil.copytree(resourcesPath, gameOutputPath + '/Data')
		shutil.copy(binariesPath + '/Mac/FOnline', gameOutputPath + '/' + gameName)
		patchConfig(gameOutputPath + '/' + gameName)

		# Tar
		makeTar(targetOutputPath + '/' + gameName + '.tar', gameOutputPath, 'w')
		makeTar(targetOutputPath + '/' + gameName + '.tar.gz', gameOutputPath, 'w:gz')

	elif args.platform == 'Android':
		shutil.copytree(binariesPath + '/Android', gameOutputPath)
		patchConfig(gameOutputPath + '/libs/armeabi-v7a/libFOnline.so')
		# No x86 build
		# patchConfig(gameOutputPath + '/libs/x86/libFOnline.so')
		patchFile(gameOutputPath + '/res/values/strings.xml', 'FOnline', gameName)

		# Icons
		logo = getLogo()
		logo.resize((48, 48)).save(gameOutputPath + '/res/drawable-mdpi/ic_launcher.png', 'png')
		logo.resize((72, 72)).save(gameOutputPath + '/res/drawable-hdpi/ic_launcher.png', 'png')
		logo.resize((96, 96)).save(gameOutputPath + '/res/drawable-xhdpi/ic_launcher.png', 'png')
		logo.resize((144, 144)).save(gameOutputPath + '/res/drawable-xxhdpi/ic_launcher.png', 'png')

		# Bundle
		shutil.copytree(resourcesPath, gameOutputPath + '/assets')
		with open(gameOutputPath + '/assets/FilesTree.txt', 'wb') as f:
			f.write('\n'.join(os.listdir(resourcesPath)))

		# Pack
		antPath = os.path.abspath(os.path.join(curPath, 'ant', 'bin', 'ant.bat'))
		r = subprocess.call([antPath, '-f', gameOutputPath, 'debug'], shell = True)
		assert r == 0
		shutil.copy(gameOutputPath + '/bin/SDLActivity-debug.apk', targetOutputPath + '/' + gameName + '.apk')

	elif args.platform == 'Web':
		# Release version
		os.makedirs(gameOutputPath)

		if os.path.isfile(os.path.join(outputPath, 'WebIndex.html')):
			shutil.copy(os.path.join(outputPath, 'WebIndex.html'), os.path.join(gameOutputPath, 'index.html'))
		else:
			shutil.copy(os.path.join(binariesPath, 'Web', 'index.html'), os.path.join(gameOutputPath, 'index.html'))
		shutil.copy(binariesPath + '/Web/FOnline.js', os.path.join(gameOutputPath, 'FOnline.js'))
		shutil.copy(binariesPath + '/Web/FOnline.wasm', os.path.join(gameOutputPath, 'FOnline.wasm'))
		shutil.copy(binariesPath + '/Web/SimpleWebServer.py', os.path.join(gameOutputPath, 'SimpleWebServer.py'))
		patchConfig(gameOutputPath + '/FOnline.wasm')

		# Debug version
		shutil.copy(binariesPath + '/Web/index.html', gameOutputPath + '/debug.html')
		shutil.copy(binariesPath + '/Web/FOnline_Debug.js', gameOutputPath + '/FOnline_Debug.js')
		shutil.copy(binariesPath + '/Web/FOnline_Debug.js.mem', gameOutputPath + '/FOnline_Debug.js.mem')
		patchConfig(gameOutputPath + '/FOnline_Debug.js.mem')
		patchFile(gameOutputPath + '/debug.html', 'FOnline.js', 'FOnline_Debug.js')

		# Generate resources
		r = subprocess.call(['python', os.environ['EMSCRIPTEN'] + '/tools/file_packager.py', \
				'Resources.data', '--preload', resourcesPath + '@/Data', '--js-output=Resources.js'], shell = True)
		assert r == 0
		shutil.move('Resources.js', gameOutputPath + '/Resources.js')
		shutil.move('Resources.data', gameOutputPath + '/Resources.data')

		# Patch *.html
		patchFile(gameOutputPath + '/index.html', '$TITLE$', gameName)
		patchFile(gameOutputPath + '/index.html', '$LOADING$', gameName)
		patchFile(gameOutputPath + '/debug.html', '$TITLE$', gameName + ' Debug')
		patchFile(gameOutputPath + '/debug.html', '$LOADING$', gameName + ' Debug')

		# Favicon
		logo = getLogo()
		logo.save(os.path.join(gameOutputPath, 'favicon.ico'), 'ico')

	else:
		assert False, 'Unknown build target'

try:
	targetOutputPath = os.path.join(outputPath, args.devname + '-' + args.configname + '-' + args.target + '-' + args.platform + ('-Debug' if args.debug else ''))
	shutil.rmtree(targetOutputPath, True)
	os.makedirs(targetOutputPath)

	build()
	
except:
	if targetOutputPath:
		shutil.rmtree(targetOutputPath, True)
	raise
