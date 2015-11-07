Places
======

Unity scope implementing the google places API

To configure
------------

open places.secret and replace APP_KEY with an app key you get from google console. Other two can be kept blank


To build
--------

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=debug ..
make
```

It's even easier from UbuntuSDK. Open the CMakeLists.txt in the root of the project. You can also test it with run.

To translate
------------

po/ contains all the translation files. The catalogue is places.pot. There are some great tutourials out there to start translating like the one [from wordpress](https://make.wordpress.org/polyglots/handbook/tools/poedit/) (but ingore the part about the .mo files, cmake builds them automaticaly)

###Translation advice

* (...)com.ubuntu.developer.labsin.places_places.ini.in.h:
  * **Places** title of the scope and main department
  * Scope description
  * Scope keywords to be searched for in the store

* (...)com.ubuntu.developer.labsin.places_places-settings.ini.in.h:

  Settings page of the scope

* src/...

  General words/phrases used in the scope

* incluse/...

  Translations of departments(types) to search for. If a search type isn't applicable to your local, translate it with only a "-".

Bug reports and merge requests are always welcome.
