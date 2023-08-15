# Flag Control

![Minimum BZFlag Version](https://img.shields.io/badge/BZFlag-v2.4.0+-blue.svg)
[![License](https://img.shields.io/github/license/Righthandson/flagControl.svg)](LICENSE.md)

When there are less players than the number determined by `_allFlagsAllowedAt` then players holding a flag listed in the config file will drop it after one kill.

## Requirements

This plug-in follows [standard instructions for compiling plug-ins](https://github.com/allejo/docs.allejo.io/wiki/BZFlag-Plug-in-Distribution).

## Usage

### Loading the plug-in

```
-loadplugin flagControl,controlledFlags.conf
```

### Configuration File

A file with a list of flags that will be "controlled". Each flag must be on a new line, e.g.
```
ST
L
GM
```

### Custom BZDB Variables

These custom BZDB variables can be configured with `-set` in configuration files and may be changed at any time in-game by using the `/set` command.

```
-set <name> <value>
```

| Name | Type | Default | Description |
| ---- | ---- | ------- | ----------- |
| `_allFlagsAllowedAt` | int | 4 | Number of players needed for plugin to no longer take effect. |

### Custom Slash Commands

| Command | Permission | Description |
| ------- | ---------- | ----------- |
| `/flagcontrol on, off, list, add, remove` | FLAGCONTROL | `on` enables the plugin, `off` disables, `list` lists all flags presently affected, `add` a flag to the list, `remove` a flag from the list |


## License

[LICENSE](LICENSE.md)
