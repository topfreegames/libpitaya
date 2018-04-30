# API
- Distinction between application errors and developer errors
Currently, the api does not make the distinction between the two kind of errors. For example, in the function `pc_client_init`, if the client passes a `NULL` pointer as first parameter, the function returns an error code. The thing is that passing a `NULL` pointer is a programmer error, therefore these things should immediately crash the client, in order to avoid the error leaking to production and to also simplify tests.

# Bugs
- Currently, calling `pc_lib_init` `pc_lib_cleanup` and after `pc_lib_init` crashes the client. It
  seems to be something related to the SSL library. This bug is currently not high priority, since `pc_lib_init` should be called only once.

# Solving things with GYP
## Specifying build types
It is not that obvious how one selects the build type. A solution that we found was to define a variable `build_type` and use it for branching on the specific build.

```
{
  'variables': {
    'build_type%': "Release",
  },
  'target_defaults': {
    'default_configuration': '<(build_type)',
    'configurations': {
      'Debug': {
        ...
      },
      'Release': {
        ...
      }
    }
  }
}
```
