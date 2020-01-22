
Webform References
==================

Provides a webform component to reference a node, term and user in webform.
It provides "Select List", "Autocomplete", "Checkbox" and "Radio" as widget.

Requirements
------------

* Webform <https://backdropcms.org/project/webform>

Installation
------------

1. Install this module using the official Backdrop CMS instructions at
   <https://backdropcms.org/guide/modules>.
2. Login as an administrator. Enable the module in the "Administer" > "Modules"
3. Create a webform node at `node/add/webform`.
4. Don't uninstall this module if there is any "Reference" type of component exists in any
Webform otherwise it will give you an error.

Configuration
-------------

* Create any Webform using Admin > Content > Add Content > Webform.
* Go to `node/[webform_id]/webform`.
* You can see the "Node Reference", "Term Reference" and "User Reference" in the component list of webform.

License
-------

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.

Maintainers
-----------

* herbdool <https://github.com/herbdool>
* Seeking additonal co-maintainers.

Credits
-------

Ported to Backdrop by herbdool <https://github.com/herbdool>.

Based on Drupal 7 module of the same name, maintained by sumitmadan <https://www.drupal.org/u/sumitmadan>.
