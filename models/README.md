# Welcome to Modeling!

The ROSS repository currently links to two model repositories:
- [A Template Model](http://github.com/nmcglohon/template-model) that can be used as a starting point for any new model.
- [A Suite of Stable Models](http://github.com/carothersc/ROSS-Models) which contains several completed models.

## Building Existing Models

To get the linked model repositories, run the following commands after cloning the ROSS repository:
```
git submodule init
git submodule update
```
Then build ROSS as you regularly would.
Be sure to turn on the option to ROSS_BUILD_MODELS in CMake (more details can be found on the [wiki page](http://github.com/carothersc/ROSS/wiki/Installation)).

## Creating Your Own Model

As you develop your model, the best practice is to do it in a separate git repostroy.
Sym-link your model into this folder and CMake will automatically find it for building.
```
cd ~/Projects/ROSS/models
ln -s ~/Projects/my-model ./
```
For more details on creating a model please check out the [wiki page](http://github.com/carothersc/ROSS/wiki/Constructing-the-Model).
