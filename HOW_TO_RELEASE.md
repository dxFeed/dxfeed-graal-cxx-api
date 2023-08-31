# How To Release

## Set the project version*

`*` - unnecessary step, because the version will be retrieved from the tag by GitHub action

- Open CMakeLists.txt
- Set the `DXFCXX_VERSION` property's value:
```cmake
set(DXFCXX_VERSION "0.1.0" CACHE STRING "The dxFeed Graal CXX API package version")
```

### Release

- Make all necessary changes to the Release Notes
- Add the changes:
```shell
git add .
```
- Commit the changes:
```shell
git commit -m "PreRelease v0.1.0-alpha"
```
or
```shell
git commit -m "Release v1.1.0"
```
- Add tag:
```shell
git tag -am "PreRelease v0.1.0-alpha" v0.1.0-alpha
```
or
```shell
git tag -am "Release v1.1.0" v1.1.0
```
- Push the changes:
```shell
git push
```
- Push the tag:
```shell
 git push --tags
```

### Build the documentation

- Go to the documentation folder:
```shell
cd docs
```
- Run the doxygen
```shell
doxygen ./Doxyfile
```
- Copy the `html` folder somewhere
- Switch to `gh-pages` git branch
```shell
git switch gh-pages 
```
- Copy contents of the "saved" `html` folder to `docs` folder
- Add the changes:
```shell
git add .
```
- Commit the changes:
```shell
git commit -m "PreRelease v0.1.0-alpha"
```
or
```shell
git commit -m "Release v1.1.0"
```
- Push the changes:
```shell
git push
```
