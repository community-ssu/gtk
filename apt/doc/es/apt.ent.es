<!-- -*- mode: sgml; mode: fold -*- -->

<!--
(c) 2003 Software in the Public Interest
Esta traducci�n ha sido realizada por Rub�n Porras Campo <nahoo@inicia.es>
Est� basada en la p�gina de manual original:
versi�n 1.3 del CVS de
/cvs/debian-doc/manpages/english/apt/apt.ent
-->

<!-- Some common paths.. -->
<!ENTITY docdir "/usr/share/doc/apt/">
<!ENTITY configureindex "<filename>&docdir;examples/configure-index.gz</>">
<!ENTITY aptconfdir "<filename>/etc/apt.conf</>">
<!ENTITY statedir "/var/lib/apt">
<!ENTITY cachedir "/var/cache/apt">

<!-- Cross references to other man pages -->
<!ENTITY apt-conf "<CiteRefEntry>
    <RefEntryTitle><filename/apt.conf/</RefEntryTitle>
    <ManVolNum/5/
  </CiteRefEntry>">

<!ENTITY apt-get "<CiteRefEntry>
    <RefEntryTitle><command/apt-get/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY apt-config "<CiteRefEntry>
    <RefEntryTitle><command/apt-config/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY apt-cdrom "<CiteRefEntry>
    <RefEntryTitle><command/apt-cdrom/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY apt-cache "<CiteRefEntry>
    <RefEntryTitle><command/apt-cache/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY apt-preferences "<CiteRefEntry>
    <RefEntryTitle><command/apt_preferences/</RefEntryTitle>
    <ManVolNum/5/
  </CiteRefEntry>">

<!ENTITY sources-list "<CiteRefEntry>
    <RefEntryTitle><filename/sources.list/</RefEntryTitle>
    <ManVolNum/5/
  </CiteRefEntry>">

<!ENTITY reportbug "<CiteRefEntry>
    <RefEntryTitle><command/reportbug/</RefEntryTitle>
    <ManVolNum/1/
  </CiteRefEntry>">

<!ENTITY dpkg "<CiteRefEntry>
    <RefEntryTitle><command/dpkg/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY dpkg-buildpackage "<CiteRefEntry>
    <RefEntryTitle><command/dpkg-buildpackage/</RefEntryTitle>
    <ManVolNum/1/
  </CiteRefEntry>">

<!ENTITY gzip "<CiteRefEntry>
    <RefEntryTitle><command/gzip/</RefEntryTitle>
    <ManVolNum/1/
  </CiteRefEntry>">

<!ENTITY dpkg-scanpackages "<CiteRefEntry>
    <RefEntryTitle><command/dpkg-scanpackages/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY dpkg-scansources "<CiteRefEntry>
    <RefEntryTitle><command/dpkg-scansources/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">

<!ENTITY dselect "<CiteRefEntry>
    <RefEntryTitle><command/dselect/</RefEntryTitle>
    <ManVolNum/8/
  </CiteRefEntry>">
    
<!-- Boiler plate docinfo section -->
<!ENTITY apt-docinfo "
 <docinfo>
   <address><email>apt@packages.debian.org</></address>
   <author><firstname>Jason</> <surname>Gunthorpe</></>
   <copyright><year>1998-2001</> <holder>Jason Gunthorpe</></>
   <date>12 March 2001</>
 </docinfo>
"> 

<!-- Boiler plate Bug reporting section -->
<!ENTITY manbugs "
 <RefSect1><Title>Fallos</>
   <para>
   Vea la <ulink url='http://bugs.debian.org/src:apt'>p�gina de fallos de APT</>.
   Si desea avisar de un fallo en APT, vea 
   <filename>/usr/share/doc/debian/bug-reporting.txt</> o la orden  &reportbug;.
 </RefSect1>
">

<!-- Boiler plate Author section -->
<!ENTITY manauthor "
 <RefSect1><Title>Autor</>
   <para>
   APT fue escrito por el equipo de APT <email>apt@packages.debian.org</>.
 </RefSect1>
">

<!-- Should be used within the option section of the text to
     put in the blurb about -h, -v, -c and -o -->
<!ENTITY apt-commonoptions "
     <VarListEntry><term><option/-h/</><term><option/--help/</>
     <ListItem><Para>
     Muestra un breve resumen del modo de uso.
     </VarListEntry>
     
     <VarListEntry><term><option/-v/</><term><option/--version/</>
     <ListItem><Para>
     Muestra la versi�n del programa.
     </VarListEntry>

     <VarListEntry><term><option/-c/</><term><option/--config-file/</>
     <ListItem><Para>
     Especifica el fichero de configuraci�n a usar.
     El programa leer� el fichero de configuraci�n por omisi�n y luego
     este otro. Lea &apt-conf; para m�s informaci�n acerca de la sintaxis.
     </VarListEntry>
     
     <VarListEntry><term><option/-o/</><term><option/--option/</>
     <ListItem><Para>
     Establece una opci�n de configuraci�n. La sintaxis es <option>-o
     Foo::Bar=bar</>. 
     </VarListEntry>
">

<!-- Should be used within the option section of the text to
     put in the blurb about -h, -v, -c and -o -->
<!ENTITY apt-cmdblurb "
   <para>
   Todas las opciones de l�nea de �rdenes pueden ser especificadas
   mediante el fichero de configuraci�n, en la descripci�n de cada opci�n
   se indica la opci�n de configuraci�n que hay que modificar. Para
   opciones booleanas puedes modificar el fichero de configuraci�n usando
   cosas parecidas a <option/-f-/,<option/--no-f/, <option/-f=no/ y alguna que
   otra variante.
   </para>
">
