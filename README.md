
# StoreStation
**Homebrew** store for the PSP (currently supporting only plugins) compatible with ARK4 <br>
Uses [RegularRabbit05/ServerStation](https://github.com/RegularRabbit05/ServerStation) as server and is currently available at [http://storestation.projects.regdev.me/](https://storestation.projects.regdev.me/) <br>
<br>
_Remember to enable WPA2 support in ARK's menu to use with modern WiFi APs._ <br>
<br>
![showcase](https://github.com/user-attachments/assets/c1520ae6-83cd-4af9-b641-cd870bcc8bae)

---

<br>

## If you are a developer and would like to submit your plugin:
1. Add a `storepkg.json` file to your repository (or create a new repository that only contains the storepkg file, for example if the code is not on GitHub) following [this template](https://github.com/StoreStation/templatePluginPkg/blob/main/storepkg.json)
2. Fork and submit a pull request to [this repository](https://github.com/StoreStation/StoreData) adding your plugin to plugins.json with the current timestamp **IN SECONDS**
3. Due to how GitHub's CDN works it might take up to 5 minutes for any change to reflect

## Themes
Every item in the assets/ folder can be replaced without rebuilding the executable. Just create a zip file containing the items you'd like to replace (for example only the font, or the buttons, etc...) <br>
The zip file must be called `psarOverride.zip` and it must be placed in the same folder as the EBOOT for it to be picked up at runtime.

## Bonus: music
3 tracks can be added to the psarOverride archive:
1. [loading_begin.mp3](https://youtu.be/GwNeuvnsSDE) (connecting to wifi)
2. [loading_end.mp3](https://www.youtube.com/watch?v=UExUcHEHu7Y&t=17s) (wifi connected)
3. [store_loop.mp3](https://youtu.be/t9jx504Wtk4) (store background loop)
Click on the files to see an example of how they will be used
