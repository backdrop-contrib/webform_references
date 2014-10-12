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
      'field_type' => '',
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
    '#required' => TRUE,
  );
  $form['extra']['field_type'] = array(
    '#type' => 'select',
    '#title' => t('Field Type'),
    '#multiple' => FALSE,
    '#default_value' => $component['extra']['field_type'],
    '#options' => array('autocomplete' => 'Autocomplete', 'select' => 'Select List'),
    '#required' => TRUE,
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
  switch ($component['extra']['field_type']) {
    case 'autocomplete':
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
      break;
    
    default:
      $node_list = _get_node_list($component['extra']['referenceable_types']);
      $form_item = array(
        '#type' => 'select',
        '#description' => $filter ? _webform_filter_descriptions($component['extra']['description']) : $component['extra']['description'],
        '#required' => $component['mandatory'],
        '#title' => $filter ? _webform_filter_xss($component['name']) : $component['name'],
        '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
        '#options' => $node_list,
        '#weight' => $component['weight'],
      );
      break;
  }
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
    preg_match('/^(?:\s*|(.*) )?\[\s*id\s*:\s*(\d+)\s*\]$/', $value, $matches);
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
    } 
    else {
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
      } 
      else {
        form_error($element, t('Please check your selection.'));
      }
    }
  }
}

/**
 * A hook for changing the input values before saving to the database.
 * Webform expects a component to consist of a single field, or a single array
 * of fields. If you have a component that requires a deeper form tree
 * you must flatten the data into a single array using this callback
 * or by setting #parents on each field to avoid data loss and/or unexpected
 * behavior.
 * Note that Webform will save the result of this function directly into the
 * database.
 *
 * @param $component
 *   A Webform component array.
 * @param $value
 *   The POST data associated with the user input.
 *
 * @return
 *   An array of values to be saved into the database. Note that this should be
 *   a numerically keyed array.
 */
function _webform_submit_node_reference($component, $value) {
  if ($component['extra']['field_type'] == 'autocomplete') {
    preg_match('/^(?:\s*|(.*) )?\[\s*id\s*:\s*(\d+)\s*\]$/', $value, $matches);
    if (!empty($matches)) {
      list(, $title, $nid) = $matches;
      $value = $nid;
    }
  }
  return $value;
}

/**
 * Implements _webform_theme_component().
 */
function _webform_theme_node_reference() {
  return array('webform_display_node_reference_field' => array('render element' => 'element'));
}

/**
 * Display the result of a submission for a component.
 * The output of this function will be displayed under the "Results" tab then
 * "Submissions". This should output the saved data in some reasonable manner.
 *
 * @param $component
 *   A Webform component array.
 * @param $value
 *   An array of information containing the submission result, directly
 *   correlating to the webform_submitted_data database table schema.
 * @param $format
 *   Either 'html' or 'text'. Defines the format that the content should be
 *   returned as. Make sure that returned content is run through check_plain()
 *   or other filtering functions when returning HTML.
 *
 * @return
 *   A renderable element containing at the very least these properties:
 *    - #title
 *    - #weight
 *    - #component
 *    - #format
 *    - #value
 *   Webform also uses #theme_wrappers to output the end result to the user,
 *   which will properly format the label and content for use within an e-mail
 *   (such as wrapping the text) or as HTML (ensuring consistent output).
 */
function _webform_display_node_reference($component, $value, $format = 'html') {
  return array(
    '#title'          => $component['name'],
    '#weight'         => $component['weight'],
    '#theme'          => 'webform_display_node_reference_field',
    '#theme_wrappers' => $format == 'html' ? array('webform_element') : array('webform_element_text'),
    '#post_render'    => array('webform_element_wrapper'),
    '#component'      => $component,
    '#format'         => $format,
    '#value'          => isset($value[0]) ? $value[0] : NULL,
  );
}

/**
 * Format the output of data for this component.
 */
function theme_webform_display_node_reference_field($variables) {
  $markup = array();
  $element = $variables['element'];
  $plain_value = check_plain($element['#value']);
  $markup['#markup'] = _get_node_title($plain_value, TRUE);
  return drupal_render($markup);
}

/*
 * Helper function to get title of node
 */
function _get_node_title($nid, $link = FALSE) {
  $node_items = db_select('node', 'n')
    ->fields('n', array('title'))
    ->condition('nid', $nid)
    ->execute()
    ->fetchAssoc();
  $title = ($link == TRUE) ? l($node_items['title'], 'node/' . $nid) : $node_items['title'];
  return $title;
}

/**
 * Return the header for this component to be displayed in a CSV file.
 * The output of this function will be displayed under the "Results" tab then
 * "Download".
 *
 * @param $component
 *   A Webform component array.
 * @param $export_options
 *   An array of options that may configure export of this field.
 *
 * @return
 *   An array of data to be displayed in the first three rows of a CSV file, not
 *   including either prefixed or trailing commas.
 */
function _webform_csv_headers_node_reference($component, $export_options) {
  $header = array();
  $header[0] = '';
  $header[1] = '';
  $header[2] = $component['name'];
  return $header;
}

/**
 * Format the submitted data of a component for CSV downloading.
 * The output of this function will be displayed under the "Results" tab then
 * "Download".
 *
 * @param $component
 *   A Webform component array.
 * @param $export_options
 *   An array of options that may configure export of this field.
 * @param $value
 *   An array of information containing the submission result, directly
 *   correlating to the webform_submitted_data database schema.
 *
 * @return
 *   An array of items to be added to the CSV file. Each value within the array
 *   will be another column within the file. This function is called once for
 *   every row of data.
 */
function _webform_csv_data_node_reference($component, $export_options, $value) {
  return !isset( $value[0] ) ? '' : _get_node_title($value[0]);
}