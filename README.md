# Flag Control

![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.0+-blue.svg)
[![License](https://img.shields.io/github/license/Righthandson/flagControl.svg)](LICENSE)

When there are less players than the number determined by `_allFlagsAllowedAt` then players holding a flag listed in the config file will drop it after one kill.

## Requirements

This plug-in follows [standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

```
-loadplugin flagControl,controlledFlags.conf
```

### Configuration File

A file with a list of flags that will be "controlled". Can be followed with a comma or space separated number. The number defines how many kills you can get before the flag is dropped. If no number is set it defaults to 1, if the number is set to 0 the flag will be dropped as soon as it is grabbed. Each flag must be on a new line, e.g.
```
ST
L 1
GM,2
CL,0
```

### Custom BZDB Variables

These custom BZDB variables can be configured with `-set` in configuration files and may be changed at any time in-game by using the `/set` command.

```
-set <name> <value>
```

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `_allFlagsAllowedAt` | int | 4 | Number of players needed for plugin to no longer take effect. |
| `_oneKillOnlyAt` | int | 0 | Number of players where all flags are limited to one kill. |

### Custom Slash Commands

| Command | Permission | Description |
| ------- | ---------- | ----------- |
| `/flagcontrol on, off, list, add, remove` | FLAGCONTROL | `on` enables the plugin, `off` disables, `list` lists all flags presently affected, `add` a flag to the list, `remove` a flag from the list |


## License and Credits

[LICENSE](LICENSE)

Thanks to zeL who contributed with the making of this.
