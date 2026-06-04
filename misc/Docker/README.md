Dockerfile to build the `ghcr.io/crashoverride85/zc95-build` image, which can be used to build the zc95 firmware binaries. Loosely based on a dockerfile & build script written by [censor](https://discord.com/channels/720377642813095956/1303831389199794176/1354199647630917633), and a [forum post](https://forums.raspberrypi.com/viewtopic.php?t=318850) by mvcorrea.

Build:

``` 
docker build -t ghcr.io/crashoverride85/zc95-build:latest .
```

Use:
```
docker run -v <path to repo>:/src --rm ghcr.io/crashoverride85/zc95-build:latest
```

e.g.

```
docker run -v /home/crashoverride/zc95:/src --rm ghcr.io/crashoverride85/zc95-build:latest
```

After it completes, the complied binaries should be in `zc95/source/CompiledUF2/`

Will only build dev & versions >=2.1.

Also see [notes on building source](../../docs/SourceBuildNotes.md).

## Changelog

### v1.1.0
Add ability to build for pico2 with command line arg

### v1.0.0
Inital version - builds for pico only
