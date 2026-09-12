#!/usr/bin/env python3
"""
Mobile-first web based remote control for the ZC95.

Unlike pattern_gui.py (tkinter - local machine only, single controller,
pattern picked upfront on the command line), this starts a small web
server that can be opened from any browser on the same network - e.g. a
phone. Multiple devices/tabs can be connected at once; they all control,
and see the state of, the same ZC95, since this process holds the single
upstream connection to the box and fans state out to every connected
browser (see lib/WebHub.py).

Requires: fastapi, uvicorn[standard] (see requirements-webgui.txt)

Usage:
    python3 webgui.py --ip 192.168.1.137

Then, from any device on the same network, browse to:
    http://<ip address of the machine running this script>:8080/
"""
import argparse
import logging
import os
from contextlib import asynccontextmanager

import uvicorn
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles

from lib.WebHub import WebHub

logger = logging.getLogger("webgui")

STATIC_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "web_static")

hub = None  # set in main(), used by the websocket_endpoint() and lifespan() closures below


@asynccontextmanager
async def lifespan(app):
  logger.info("Connecting to ZC95 at %s", hub.ip)
  await hub.connect()
  logger.info("Connected")
  try:
    yield
  finally:
    await hub.close()


app = FastAPI(lifespan=lifespan)


@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
  await websocket.accept()
  await hub.add_client(websocket)
  try:
    while True:
      message = await websocket.receive_json()
      await hub.handle_client_message(message)
  except WebSocketDisconnect:
    pass
  finally:
    hub.remove_client(websocket)


# Mounted last so it doesn't shadow the /ws route above; serves web_static/index.html at "/"
app.mount("/", StaticFiles(directory=STATIC_DIR, html=True), name="static")


def main():
  global hub

  parser = argparse.ArgumentParser(description="Web based remote control for ZC95")
  parser.add_argument("--ip", required=True, help="IP address of ZC95")
  parser.add_argument("--port", type=int, default=8080, help="Port to serve the web interface on (default: 8080)")
  parser.add_argument("--bind", default="0.0.0.0",
                       help="Address to bind the web server to (default: 0.0.0.0, i.e. all interfaces, so it's reachable from your phone)")
  parser.add_argument("--debug", action="store_true", help="Show messages sent/received to/from the ZC95")
  args = parser.parse_args()

  logging.basicConfig(level=logging.INFO)
  hub = WebHub(args.ip, args.debug)

  print("Starting web interface - browse to http://<ip of this machine>:" + str(args.port) + "/ from your phone/tablet/PC")
  uvicorn.run(app, host=args.bind, port=args.port)


if __name__ == "__main__":
  main()
