# Usage
Information about using the library is located [here](Assets/Pitaya/README.md).

# Contributing
In order to contribute, the developer should implement the feature on a different branch and run the tests locally, including a minimum of one test for the new feature implemented. Then a pull request should be made.

## Running the tests
In order the run the tests, the user can open the unity project located in `unity/PitayaExample` (preference to Unity 2018.2) and open the tests window in `Window > General > Test Runner`.

## Making a new release
The script `release.py` in the root folder can be used to see the current version of the library and used to create a new one as well, example: `./release.py check --unity`, `./release.py new --unity 1.2.3`.

In order to trigger a build with jenkins, see [this](https://wiki.tfgco.com/...) link.