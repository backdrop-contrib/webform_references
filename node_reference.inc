<?php

/**
 * @file
 * Webform module node reference component.
 */

/**
 * Implements _webform_defaults_component().
 */
function _webform_defaults_node_reference() {
    return array(
        'name' => '',
        'form_key' => NULL,
        'mandatory' => 0,
        'pid' => 0,
        'weight' => 0,
        'extra' => array(
            'title_display' => 0,
            'private' => FALSE,
            'attributes' => array(),
            'description' => '',
            'referenceable_types' => array(),
        ),
    );
}

/**
 * Generate the form for editing a component.
 * Create a set of form elements to be displayed on the form for editing this
 * component. Use care naming the form items, as this correlates directly to the
 * database schema. The component "Name" and "Description" fields are added to
 * every component type and are not necessary to specify here (although they
 * may be overridden if desired).
 *
 * @param $component
 *   A Webform component array.
 *
 * @return
 *   An array of form items to be displayed on the edit component page
 */
function _webform_edit_node_reference($component) {
    $form = array();

    $form['extra']['referenceable_types'] = array(
        '#type' => 'checkboxes',
        '#title' => t('Content types that can be referenced'),
        '#multiple' => TRUE,
        '#default_value' => $component['extra']['referenceable_types'],
        '#options' => array_map('check_plain', node_type_get_names()),
        '#required' => true,
    );

    return $form;
}

/**
 * Render a Webform component to be part of a form.
 *
 * @param $component
 *   A Webform component array.
 * @param $value
 *   If editing an existing submission or resuming a draft, this will contain
 *   an array of values to be shown instead of the default in the component
 *   configuration. This value will always be an array, keyed numerically for
 *   each value saved in this field.
 * @param $filter
 *   Whether or not to filter the contents of descriptions and values when
 *   rendering the component. Values need to be unfiltered to be editable by
 *   Form Builder.
 *
 * @see _webform_client_form_add_component()
 */
function _webform_render_node_reference($component, $value = NULL, $filter = TRUE) {
    //TODO: change these to use non-private functions (no _) if/when webform 3.x is entirely deprecated
    $form_item = array(
        '#type' => 'textfield',
        '#autocomplete_path' => 'webform_references/autocomplete/' . implode('+', array_filter($component['extra']['referenceable_types'])),
        '#description' => $filter ? _webform_filter_descriptions($component['extra']['description']) : $component['extra']['description'],
        '#required' => $component['mandatory'],
        '#title' => $filter ? _webform_filter_xss($component['name']) : $component['name'],
        '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
        '#weight' => $component['weight'],
        '#element_validate' => array('webform_validate_node_reference'),
    );

    return $form_item;
}

/**
 * Validation Callback for phone field
 */
function webform_validate_node_reference($element, $form_state) {
    $value = $element['#value'];
    $nid = NULL;
    if (!empty($value)) {
        // Check whether we have an explicit "[nid:n]" input.
        preg_match('/^(?:\s*|(.*) )?\[\s*nid\s*:\s*(\d+)\s*\]$/', $value, $matches);
        if (!empty($matches)) {
            list(, $title, $nid) = $matches;
            if (!empty($title)) {
                $real_title = db_select('node', 'n')
                        ->fields('n', array('title'))
                        ->condition('n.nid', $nid)
                        ->execute()
                        ->fetchField();
                if (trim($title) != trim($real_title)) {
                    form_error($element, t('Please check your selection.'));
                }
            }
        } else {
            // No explicit nid (the submitted value was not populated by autocomplete
            // selection). Get the nid of a referencable node from the entered title.
            $options = array(
                'string' => $value,
                'match' => 'equals',
                'limit' => 1,
            );
            $bundle_args = explode('/', $element['#autocomplete_path']);
            $bundles = (isset($bundle_args[2])) ? $bundle_args[2] : array();
            $references = webform_references_potential_references($bundles, $options);
            if ($references) {
                // @todo The best thing would be to present the user with an
                // additional form, allowing the user to choose between valid
                // candidates with the same title. ATM, we pick the first
                // matching candidate...
                $nid = key($references);
            } else {
                form_error($element, t('Please check your selection.'));
            }
        }
    }
}

