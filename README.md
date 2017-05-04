# Substance NG Gem for Lumberyard

SubstanceNG is a re-implementation og the Substance Gem provided by default for [Lumberyard](https://aws.amazon.com/lumberyard/).

I initiated this project when we realized while working on Lumberyard game that the default Substance Gem had serious limitations: mainly, it seemed to be used an old version of the substance engine, and thus, could not be used to load substance archives generated from Substance Designer 6 for instance (some simple substances could still work, but not the one using the new substance engine 6 features (?)).

So I started with some investigation on the available Susbtance plugins for the major engines, and in this process, found the [Susbtance plugin for Unreal Engine 4](https://www.allegorithmic.com/buy/download). I downloaded this plugin, and analyzed the sources which were using a completely different API to access the substance engine. Turned out this version was using the substance engine 6.

Thus I started with a copy of the Substance Gem sources, and updated them progressively to incorporate this latest version of the Substance engine, and get ride of the all version in parallel.

Note that this Gem was developed with Lumberyard version 1.8.0.1 (beta) but it should also work with version 1.9.0.0. It was built on Windows 10 x64 with Visual Studio 201 Community Edition (only the libraries for **win64** are provided here for the susbtance framework).

For more information on this project please check [the project homepage](http://wiki.nervtech.org/doku.php?id=public:projects:substanceng:substanceng).

## New features in this version

  * Support for substance archives generated with Substance engine version 6
  * Support for emissive outputs from substance archives.
  * Support for Lumberyard material reloading when input parameters are changed and saved on a given substance.

## Limitations this version

  * For the moment, many (most) of the flow graph nodes available in the legacy Substance Gem are not available yet in this Susbtance NG gem.
  * To open the **Substance NG Editor** you must currently use the **Tools>Plugins>Substange NG Editor** menu item, the "Substance Editor** toolbar button will not work with this version.

## Gem Usage

  1. To use this gem, checkout those sources in your lumberyard gem folder (for instance **Lumberyard/1.8.0.1/dev/Gems**), creating a folder called (for instance) **SubstanceNG**
  1. Open the **Lumberyard project configurator** and set your project of interest as **default**.
  1. Select **Enable Gems** for this project,
  1. First be sure to unselect the legacy **Allegorithmic Substance** plugin
  1. Select the **Substance NG Plugin** plugin
  1. Rebuild your project with its gems with a command such as:
    ```
    lmbr_waf build_win_x64_vs2015_profile -p all
    ```
## License

  To be clarified soon with Amazon and Allegorithmic.
