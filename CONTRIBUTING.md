# Contributing

There are many ways to contribute to ROSS:

- Create and release a model.
  Like any simulation engine, ROSS is always looking for new models and new model developers.
  This is also the best way to learn about ROSS and its API.
- File a bug or request a feature through [GitHub Issues](http://github.com/carothersc/ROSS/issues).
  We are always looking to improve ROSS to make it more stable for our users.
  Feature requests and related discussions are located here as well.
- The best way to ensure a bug or feature request is addressed is to do it yourself!
  Spelunking through the ROSS core can be a enlightening journey.
  Once you've made the change, feel free to create a [pull request](https://github.com/carothersc/ROSS/pulls).
  Between our continuous integration testing and our experienced ROSS core team, we will ensure your change is safe before deploying it to the master branch.

## Small Changes

Development on the ROSS core is done through [GitHub Pull Requests](https://help.github.com/articles/using-pull-requests/).
We always welcome small-change contributions to ROSS, including:

- clarification of error/warning messages
- bug fixes (hopefully there aren't any bugs to begin with!)
- whitespace or code-style changes
- other straight-forward changes that do not have wide-reaching consequences

## Major Changes and Features

ROSS is being continually developed and we are frequently adding new features.
For these larger changes of ROSS, there are a few boxes that must be checked before any pull request is merged into the master branch.

1. Ensure current tests pass
2. Ensure coverage increases
3. Ensure dependent projects are updated (needed for API changes)
4. Document the change though a blog post

### Continuous Integration Testing and Coverage

First, the new feature or major change must pass all of the existing TravisCI tests.

Next, the test coverage must increase (or at least stay the same).
For new features, this usually means that a new test must written.
There are typically two options for a test:
- Add a new test to PHOLD model (see [models/phold/CMakeLists.txt](https://github.com/carothersc/ROSS/blob/master/models/phold/CMakeLists.txt)).
- Create a new model which tests your feature and add this model to the ROSS-Models repository.

### ROSS Model Changes

The [ROSS-Models repository](http://github.com/carothersc/ROSS-Models) contains models which are no longer under development.
If your new feature is a major API change to ROSS, the models in this repository must be updated.
The workflow to update the ROSS-Models submodule is as follows:

1. In your feature branch of ROSS, load the submodules
```
	git submodule init
	git submodule update
```
2. Move into the `models/ROSS-Models/` directory.
   Make the appropriate API changes and commit them using ROSS.
3. While within this directory, upload these changes to GitHub using the typical `git push origin master` command.
4. Move back up to the base ROSS directory.
   You should see the changed commit hash for the ROSS-Models submodule when you run a `git status`.
   Commit this change in hash number using `git commit -am "updated ROSS-Models"`.

### CODES

The [CODES Project](http://press3.mcs.anl.gov/codes/) is actively developed and depends on ROSS as its underlying simulation engine.
The CODES repository can be found [here](https://xgitlab.cels.anl.gov).
You should be able to login in to ANL's GitLab service.
Here you can fork the CODES repository and create a pull request with any required changes.

### Documentation

In order to keep our documentation up-to-date, any new feature or major change must be documented before it is merged into the master branch.
The easiest way to document the change is to create a new blog post for our website.
The [contributing guide](https://github.com/carothersc/ROSS/blob/gh-pages/CONTRIBUTING.md) in our gh-pages branch documents this process.

## Versioning and New Releases

ROSS does not utilize a numbered-version system.
Instead, each commit on the master branch represents a change in ROSS.
Thus, each commit hash can be used as a version number that we guarantee will never change.

To achieve the eternal validity of a commit hash, we utilize squash commits to merge any changes.
All merges into the master branch should be made through the GitHub pull request interface.
Through this interface, the merge can be squashed.
Squash commits have several implications:

1. *The squash-on-merge option must be selected within the GitHub interface by the person doing the merge.*
1. The individual commits are not placed in the history of the master branch.
   However, they do remain available through the pull request page.
2. One positive outcome is that the blame on any file will be simplified since there is now only one commit associated with the entire change.
3. Once a feature branch is merged into master, it should be **deleted from any local repositories**.
   There are possible issues if someone attempts to re-merge the branch (including commits previously added in a squash).
