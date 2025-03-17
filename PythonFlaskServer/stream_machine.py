#!/usr/bin/python3

'''The Stream Machine streams a directory of audio files over
http. These are played by the converted Zenith Internet Radio,
although any local client could use this service.

You will find that this doesn't run straight away on your computer. It
depends on a number of things. Just look at that import list! In
addition, it requires ffmpeg and ffprobe to be installed on the computer
that runs this script.  Getting all the python libraries and ffmpeg
install might be too much for some people and that's okay.  The Zenith
Voice Controlled Radio is still very useful without streaming local
music and podcasts.  Also, you can stream some podcasts (mp3) directly
as if it is an Internet radio station -- you just won't have the
abilty to skip ahead by pressing the Zenith tuner button.

 by J. King, 14 Feb 2025.
 Copyright (c) GPL-3.0, Feb 2025
 $Revision: 1.25 $
 $Date: 2025/03/17 21:06:16 $

'''
import os
import sys
import argparse
import subprocess
import threading
from flask import Flask, Response, request, render_template, jsonify
import time, random
import requests
from gevent.pywsgi import WSGIServer
from zeroconf import Zeroconf, ServiceInfo
import socket
import time

'''PODCAST_URL is the download URL -- it is not the stream that is played!

NB, this typically won't be used at all because it's best to download
the podcast nightly using cron or some other service on the local
computer that runs this program.  Chance are you can ignore this
setting altogether.

SAVE_PATH is where this download gets saved.  If used, it must be the
same as PODCAST.
'''

PODCAST_URL = "https://waaa.wnyc.org/758af4c0-a2c3-47ec-a2d8-05f41bfbde51/episodes/5ffe5d55-2d4a-45e0-99ce-73b817517463/audio/128/default.mp3/default.mp3_ywr3ahjkcgo_dd7873d7d760e3a45543f7d1d54f3626_68425097.mp3?aid=rss_feed&awCollectionId=758af4c0-a2c3-47ec-a2d8-05f41bfbde51&awEpisodeId=5ffe5d55-2d4a-45e0-99ce-73b817517463&feed=EmVW7VGp&hash_redirect=1&x-total-bytes=68425097&x-ais-classified=streaming&listeningSessionID=0CD_382_68__8462494ec8be0f182a454ce621ce91f16483867f"

SAVE_PATH = "/home/jking/Music/DailyPodcast/podcast.mp3"

'''
Here we have the key setings:
    MUSIC_DIR is where you local music directories reside. We expect
              to find directories named for artists there and below
              each of those one or directories corresponding to an
              album.

    PODCAST is file path of the downloaded podcast on the computer
            running this script.
'''

MUSIC_DIR = "/home/jking/Music"
PODCAST = "/home/jking/Music/DailyPodcast/podcast.mp3"

# local IP and hostname
hostname = socket.gethostname()
local_ip = socket.gethostbyname(hostname)

# define the DNS service using mDNS
service_info = ServiceInfo(
    "_http._tcp.local.",
    "ZenithVCR._http._tcp.local.",
    addresses=[socket.inet_aton(local_ip)],
    port=8001,
    properties={},
    server="zenithvcr.local."
)

# start mDNS
zeroconf = Zeroconf()
zeroconf.register_service(service_info)

''' get_track_number

  We really want to play the album as intended so we poke into each
  audio file to try to get the track number from the metadata.
  Unfortunately, there are multiple metadata formats and standards. By
  trial and error we settled on this approach is was detemined to work
  for two of the possible formats.

  @param filepath location of an mp3/ogg file
  @return an integer corresponding to the track number

'''

def get_track_number(filepath):
    """Extracts track number metadata from MP3 and OGG files."""
    try:
        result = subprocess.run(
            ["ffprobe", "-v", "quiet", "-show_format",
             "-show_streams", filepath],
            capture_output=True, text=True, check=True
        )
        
        # Look for track number in the output
        for line in result.stdout.splitlines():
            if "track=" in line.lower():  # Match case-insensitive
                return int(line.split("=")[1].split("/")[0])  # Handle "2/12"
            
    except Exception as e:
        print(f"Error reading track number for {filepath}: {e}")
    
    return 9999  # Default if no track number found

''' get_sorted_playlist

  Play the album tracks in the intended order. This function returns a
  list of mp3/ogg files in track order.

  @param path the path to the album.  The mp3/ogg file names in this
         directory are sorted according to track number.

  @return a list of sorted file names (without path information).
'''

def get_sorted_playlist(path):
    # Gets all MP3 and OGG files sorted by track number.
    files = [f for f in os.listdir(path) if f.endswith((".mp3", ".ogg"))]
    files.sort(key=lambda f: get_track_number("/".join([path, f])))
    return files

#########  STARTUP SETTINGS
# populate and define some of our globals

# get the entire playlist in track order starting with this album
try:
    lastPlayedF = open(os.path.join(MUSIC_DIR, "LastPlaying.txt"), "r")
    nowPlayingArtist = (lastPlayedF.readline()).strip()
    nowPlayingAlbum  = (lastPlayedF.readline()).strip()
    lastPlayedF.close()
except FileNotFoundError:
    nowPlayingArtist = "Eric Clapton"
    nowPlayingAlbum  = "MTV Unplugged"
    
current_track_index = 0  # Global track index
ffmpeg_process = None    # Global player process
first = True             # We increment the track index upon entry,
                         # except for the fist time
    
playList = get_sorted_playlist("/".join([MUSIC_DIR,
                                         nowPlayingArtist,
                                         nowPlayingAlbum]))
# keep us sane
print(playList, file=sys.stderr)

##################### HTTP SERVER SECTION #########################

################### ZenithStream server on port 8000
# this is the streaming server which runs on port 8000
stream_app = Flask("ZenithStream")

''' /stream service

  Stream local music.  This uses start_stream() to read the file and
  spew audio frames to the client.
'''
@stream_app.route('/stream')
def stream():
    global current_track_index
    global first
    global playList
    
    if not first:
        current_track_index = (current_track_index + 1) % len(playList)
    first = False

    print("Playing ", playList[current_track_index], file=sys.stderr)
    
    musicFile = "/".join([MUSIC_DIR,
                          nowPlayingArtist,
                          nowPlayingAlbum,
                          playList[current_track_index]])
    
    def generate(musicFile):
        cmd = ["ffmpeg", "-i", musicFile, "-f", "mp3", "-"]
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                   stderr=subprocess.DEVNULL)

        while chunk := process.stdout.read(1024):
            yield chunk

    return Response(generate(musicFile), mimetype="audio/mpeg")

''' /podcast service

  Stream a podcast saved to the computer running this script.
  Ideally, crontab downloaded the podcast weekly or by some other
  schedule.  NB: there is a specific location for the podcast, be sure
  to set this up along with your crontab.

'''

@stream_app.route('/podcast')
def serve_podcast():
    # Pushing the tuner button in, will advance the playback by 1
    # minute. The start_time is passed in with /podcast by the esp32
    # and we let ffmpeg handle the rest.
    
    start_time = request.args.get('start', default=0, type=int)
    
    def generate():
        cmd = ["ffmpeg","-ss", str(start_time), "-i", PODCAST, "-f", "mp3", "-"]
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                   stderr=subprocess.DEVNULL)

        while chunk := process.stdout.read(1024):
            yield chunk

    return Response(generate(), mimetype="audio/mpeg")

#################### ZenithAlbums server on port 8001
# this is the album selector service running or port 8001

web_app = Flask("ZenithAlbums")

'''/browse web service

  Use this from a computer. The ip is the address of the computer
  running this script and the port is 8001.  The ESP32 doesn't use
  this server. The /browse service returns a web interface for selecting
  which album to play.

  NB: the html is contained in a template file (stream_machine.html)
  which must be located in a subdirectory called "templates" and is
  located in the directory in which this script resides.

'''

@web_app.route('/')
@web_app.route('/browse')
def browse_music():
    global nowPlayingArtist
    global nowPlayingAlbum
    artists = sorted(next(os.walk(MUSIC_DIR))[1])
    return render_template("stream_machine.html",
                           artists=artists,
                           nowPlayingArtist=nowPlayingArtist,
                           nowPlayingAlbum=nowPlayingAlbum)

''' /albums/artist

  This service responds to artist selections.  It populates the album
  select list.
'''

@web_app.route('/albums/<artist>')
def get_albums(artist):
    artist_path = os.path.join(MUSIC_DIR, artist)
    if not os.path.exists(artist_path):
        return jsonify({"error": "Artist not found"}), 404

    albums = sorted(next(os.walk(artist_path))[1])
    return jsonify(albums)

''' /play/artist/album

  This service responds to a Play button press. It populates the
  Tracks list, the Now Playing status and repopulates the playList
  variable which determines which audio file will be streamed next.
  NB: this only changes the audio queue (what plays next), it does not
  stop audio that is currently playing.  Press the Zenith tuner button
  in to advance playback or wait for the current audio file to
  complete.

'''

@web_app.route('/play/<artist>/<album>')
def get_tracks(artist, album):
    global playList
    global current_track_index
    global nowPlayingArtist
    global nowPlayingAlbum
    global first

    # ready to switch to this artist and album
    nowPlayingArtist = artist
    nowPlayingAlbum = album

    # start with the first track of the album
    first = True
    
    # Return track list for the selected album.
    album_path = os.path.join(MUSIC_DIR, artist, album)
    if not os.path.exists(album_path):
        return jsonify({"error": "Album not found"}), 404

    # this updates the playList, thereby replacing the queue with this one
    playList = get_sorted_playlist(album_path);
    current_track_index = 0

    # this is populated to fill in the queue shown on the web page
    tracks = [f for f in playList if f.endswith(('.mp3', '.ogg'))]
    
    return jsonify(tracks)

''' /download

  This is here only as an example of one way to get the podcast
  downloaded, although it did work when tested.  Instead of this, use
  your system's crontab.  Here's an example crontab entry for
  downloading the weekly radiolab podcast,

# weekly (sunday) podcast download at 2:01 AM
2 1 * * 0 /usr/bin/curl -L -o /home/jking/Music/DailyPodcast/podcast.mp3 'https://pscrb.fm/rss/p/mgln.ai/e/14/prfx.byspotify.com/e/dts.podtrac.com/pts/redirect.mp3/waaa.wnyc.org/758af4c0-a2c3-47ec-a2d8-05f41bfbde51/episodes/5ffe5d55-2d4a-45e0-99ce-73b817517463/audio/128/default.mp3?aid=rss_feed&awCollectionId=758af4c0-a2c3-47ec-a2d8-05f41bfbde51&awEpisodeId=5ffe5d55-2d4a-45e0-99ce-73b817517463&feed=EmVW7VGp' 2>>/home/jking/logs/podcast_download.log

  Be sure to use however you defined PODCAST above for the output file
  instead of /home/jking/Music/DailyPodcast/podcast.mp3.

'''

@web_app.route('/download')
def download_podcast():
    try:
        response = requests.get(PODCAST_URL, stream=True);
        if response.status_code == 200:
            with open(SAVE_PATH, "wb") as f:
                for chunk in response.iter_content(1024):
                    f.write(chunk)
            return "Podcast downloaded.", 200
        else:
            return "Failed to download podcast! {response.status.code}", 500
    except Exception as e:
        return f"Error: {str(e)}", 500

def run_streaming_server():
    http_server = WSGIServer(("0.0.0.0", 8000), stream_app)
    http_server.serve_forever()
    
def run_web_server():
    http_server = WSGIServer(("0.0.0.0", 8001), web_app)
    http_server.serve_forever()    
    
if __name__ == "__main__":
    threading.Thread(target=run_streaming_server, daemon=True).start()
    threading.Thread(target=run_web_server, daemon=True).start()
    try:
        print("Zenith Album Browser at http://zenithvcr.local:8001/browse")
        while True:
            pass
    except KeyboardInterrupt:
        print("Shutting down mDNS...")
        zeroconf.unregister_service(service_info)
        zeroconf.close()
        print("Saving the currently playing artist and album...")
        try:
            lastPlayed = open(os.path.join(MUSIC_DIR, "LastPlaying.txt"), "w")
            print(nowPlayingArtist, file=lastPlayed)
            print(nowPlayingAlbum,  file=lastPlayed)
            lastPlayed.close()
        except Exception as e:
            print("Failed to save currently playing artist and album!",
                  file=sys.stderr)
            print("Error: ", e, file=sys.stderr)
            
