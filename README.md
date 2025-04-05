# ACShadowsFix
[![Patreon-Button](https://github.com/Lyall/ACShadowsFix/blob/main/.github/Patreon-Button.png?raw=true)](https://www.patreon.com/Wintermance) 
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)<br />
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/ACShadowsFix/total.svg)](https://github.com/Lyall/ACShadowsFix/releases)

**ACShadowsFix** is an ASI plugin for Assassin's Creed Shadows that can:
- Skip intro videos.
- Extend in-game FOV slider.
- Disable framerate cap in cutscenes/FMVs/menus.
- Dynamic cloth physics framerate.
- Allow cutscene frame generation.
- Disable pillarboxing/letterboxing in cutscenes.

## Installation  
- Download the latest release from [here](https://github.com/Lyall/ACShadowsFix/releases). 
- Extract the contents of the release zip in to the the game folder.  

### Steam Deck/Linux Additional Instructions
🚩**You do not need to do this if you are using Windows!**  
- Open the game properties in Steam and add `WINEDLLOVERRIDES="winmm=n,b" %command%` to the launch options.  

## Configuration
- Open **`ACShadowsFix.ini`** to adjust settings.

## Screenshots
| ![ezgif-7bc58187476248](https://github.com/user-attachments/assets/ea03950e-6a38-4253-8acd-4ba66c91374f) |
|:--:|
| Cutscene |

## Credits
[Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) for ASI loading. <br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[spdlog](https://github.com/gabime/spdlog) for logging. <br />
[safetyhook](https://github.com/cursey/safetyhook) for hooking.
