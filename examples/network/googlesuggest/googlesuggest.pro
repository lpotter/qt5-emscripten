QT      +=  network widgets
requires(qtConfig(itemviews))
SOURCES =   main.cpp searchbox.cpp googlesuggest.cpp
HEADERS  =  searchbox.h googlesuggest.h

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/googlesuggest
INSTALLS += target
