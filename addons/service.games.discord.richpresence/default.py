import xbmc
import discordIPC
import time

CLIENT_ID = '750797452453478629'

if __name__ == "__main__":
    monitor = xbmc.Monitor()
    player = xbmc.Player()
    discord = None
    startTime = -1
    gameStarted = False

    while True:
        monitor.waitForAbort(5)
        if monitor.abortRequested():
            break
        
        if player.isPlayingGame():
            if not gameStarted:
                discord = discordIPC.DiscordIPC(CLIENT_ID)
                startTime = int(time.time())
                gameStarted = True

            data = player.getGameInfoTag()
            activity = {
                'assets': {'large_image': 'kodi', 'large_text': 'Kodi'},
                'state': data.getTitle(),
                'details': data.getCaption(),
                'timestamps': {'start': startTime}
            }
            discord.update_activity(activity)
        else:
            if gameStarted:
                discord.close()
                startTime = -1
                gameStarted = False
