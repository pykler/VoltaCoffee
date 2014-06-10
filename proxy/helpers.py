import gdata.youtube.service
import random


def get_popular_video():
    yt_service = gdata.youtube.service.YouTubeService()
    yt_service.ssl = True
    feed = yt_service.GetYouTubeVideoFeed(
        'https://gdata.youtube.com/feeds/api/standardfeeds/most_popular'
    )
    return random.choice([
        p.media.player.url
        for p in feed.entry
    ])
