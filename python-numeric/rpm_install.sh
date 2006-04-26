/usr/bin/python2.4 setup.py install --root=$RPM_BUILD_ROOT

cat >INSTALLED_FILES <<EOF
%doc Demo
EOF
find $RPM_BUILD_ROOT -type f | sed -e "s|$RPM_BUILD_ROOT||g" >>INSTALLED_FILES

