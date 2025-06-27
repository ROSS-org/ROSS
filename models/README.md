# Welcome to Modeling!

ROSS comes loaded with one model Phold. This model is often used to test the speed of ROSS in new systems, specially HPC systems. To find out more ROSS example models, please check out the repos below:

- [A Template Model](https://github.com/ROSS-org/template-model) that can be used as a starting point for any new model.
- [Bare-bones postal network of mailboxes](https://github.com/nmcglo/ROSS-Mail-Model) made with the intent on mind of teaching ROSS to beginners.
- [A Game of Life example](https://github.com/helq/highlife-ross). The goal of this example is to show case how to build a model where ROSS is linked as library (opposed to building the model inside of ROSS' space).
- [(Deprecated) A Suite of Stable Models](https://github.com/ROSS-org/ROSS-Models) which contains several completed models. These might need some tweaking before they can be run as they were written for legacy ROSS.

## Creating Your Own Model

As you develop your model, the best practice is to do it in a separate git repostroy.
Sym-link your model into this folder and CMake will automatically find it for building.
```
cd ~/Projects/ROSS/models
ln -s ~/Projects/my-model ./
```
For more details on creating a model please check out ROSS' documentation page.
