# Worldwide-Weather-Watcher

![](https://github.com/MaxLinkle/Worldwide-Weather-Watcher/blob/master/carte.jpg)

### Projet Système Embarqué : CPI A2 Groupe 1 (Maxime, Ray-hann, Catarina, Antoine)

## Makefile

Pour pouvoir téléverser le code sur la carte arduino il vous faudra utiliser le makefile mis a votre disposition.
Pour cela dirigé vous dans le bon dossier puis exécuté dans votre inviter de comamnde la commande : `make chemin=$(pwd)`  

Vous pouvez aussi si vous le désirer uniquement compiler votre fichier sans avoir à le téléverser, pour cela il vous suffit d'exécuter la commade : `make chemin=$(pwd) compilation`

**/!\ Attention** : N'oubliez pas de bien mettre votre arduino sur le bon port. Si ce n'est pas le cas utiliser la commande `make chemin=$(pwd) com=X` avec X qui correspondra à votre nouveau port.

## libraries

Dans le dossier ***libraries*** nous avons placer les bibliothèques de bases et celles que nous avons modifié afin de gagner en espace.  
Celle ci seront directement appelé par notre makefile.

## pin

Dans le dossier ***pin*** nous avons placer la librairie qui permet de dire à l'arduino qu'elle pin il possède.  
Cette librairie est également appelé directement dans le makefile.

## main.ino

Il s'agit du code principal. 
Pour plus d'information à sont sujet n'hésité pas à aller voire les commentaires dans le code en lui même.
Ils vous expliqueront plus en détail l'utilité des fonctions et différentes oppération que nous effectuons. 
