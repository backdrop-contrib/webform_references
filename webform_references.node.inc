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
    'required'  => 0,
    'mandatory' => 0,
    'pid' => 0,
    'weight' => 0,
    'extra' => array(
      'title_display' => 0,
      'private' => FALSE,
      'attributes' => array(),
      'description' => '',
      'referenceable_types' => array(),
      'widget_type' => '',
    ),
  );
}

/**
 * Generate the form for editing a component.
 *
 * Create a set of form elements to be displayed on the form for editing this
 * component. Use care naming the form items, as this correlates directly to the
 * database schema. The component "Name" and "Description" fields are added to
 * every component type and are not necessary to specify here (although they
 * may be overridden if desired).
 *
 * @param array $component
 *   A Webform component array.
 *
 * @return array
 *   An array of form items to be displayed on the edit component page
 */
function _webform_edit_node_reference($component) {
  $form = array();
  $form['extra']['referenceable_types'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Content types that can be referenced'),
    '#multiple' => TRUE,
    '#default_value' => $component['extra']['referenceable_types'],
    '#options' => node_type_get_names(),
    '#required' => TRUE,
  );
  $form['extra']['widget_type'] = array(
    '#type' => 'select',
    '#title' => t('Field Type'),
    '#multiple' => FALSE,
    '#default_value' => $component['extra']['widget_type'],
    '#options' => array('select' => t('Select List'), 'autocomplete' => t('Autocomplete')),
    '#required' => TRUE,
  );
  return $form;
}

/**
 * Render a Webform component to be part of a form.
 *
 * @param array $component
 *   A Webform component array.
 * @param array $value
 *   If editing an existing submission or resuming a draft, this will contain
 *   an array of values to be shown instead of the default in the component
 *   configuration. This value will always be an array, keyed numerically for
 *   each value saved in this field.
 * @param string $filter
 *   Whether or not to filter the contents of descriptions and values when
 *   rendering the component. Values need to be unfiltered to be editable by
 *   Form Builder.
 *
 * @see _webform_client_form_add_component()
 */
function _webform_render_node_reference($component, $value = NULL, $filter = TRUE) {
  switch ($component['extra']['widget_type']) {
    case 'autocomplete':
      $form_item = array(
        '#type' => 'textfield',
        '#autocomplete_path' => 'webform_references/node/autocomplete/' . implode('+', array_filter($component['extra']['referenceable_types'])),
        '#description' => $filter ? _webform_filter_descriptions($component['extra']['description']) : $component['extra']['description'],
        '#required'         => $component['required'] || $component['mandatory'],
        // Either one being true will could as required...
        // because webform changed in 4.x-alpha8
        '#title' => $filter ? _webform_filter_xss($component['name']) : $component['name'],
        '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
        '#weight' => $component['weight'],
        '#element_validate' => array('webform_references_validate_node_reference'),
        '#theme_wrappers' => array('webform_element'),
      );
      break;

    default:
      $node_list = _webform_references_get_node_list($component['extra']['referenceable_types']);
      $form_item = array(
        '#type' => 'select',
        '#description' => $filter ? _webform_filter_descriptions($component['extra']['description']) : $component['extra']['description'],
        '#required'         => $component['required'] || $component['mandatory'],
        // Either one being true will could as required...
        // because webform changed in 4.x-alpha8.
        '#title' => $filter ? _webform_filter_xss($component['name']) : $component['name'],
        '#title_display' => $component['extra']['title_display'] ? $component['extra']['title_display'] : 'before',
        '#options' => $node_list,
        '#weight' => $component['weight'],
        '#theme_wrappers' => array('webform_element'),
      );
      break;
  }
  return $form_item;
}

/**
 * Validation Callback for node reference field.
 */
function webform_references_validate_node_reference($element, $form_state) {
  $value = $element['#value'];
  $nid = NULL;
  if (!empty($value)) {
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
      $options = array(
        'string' => $value,
        'match' => 'equals',
        'limit' => 1,
      );
      $bundle_args = explode('/', $element['#autocomplete_path']);
      $bundles = (isset($bundle_args[2])) ? $bundle_args[2] : array();
      $references = webform_references_potential_references($bundles, $options);
      if ($references) {
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
 *
 * Note that Webform will save the result of this function directly into the
 * database.
 *
 * @param array $component
 *   A Webform component array.
 * @param string $value
 *   The POST data associated with the user input.
 *
 * @return string
 *   A string of value to be saved into the database.
 */
function _webform_submit_node_reference($component, $value) {
  if ($component['extra']['widget_type'] == 'autocomplete') {
    preg_match('/^(?:\s*|(.*) )?\[\s*id\s*:\s*(\d+)\s*\]$/', $value, $matches);
    if (!empty($matches)) {
      list(, , $nid) = $matches;
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
 *
 * The output of this function will be displayed under the "Results" tab then
 * "Submissions". This should output the saved data in some reasonable manner.
 *
 * @param array $component
 *   A Webform component array.
 * @param array $value
 *   An array of information containing the submission result, directly
 *   correlating to the webform_submitted_data database table schema.
 * @param string $format
 *   Either 'html' or 'text'. Defines the format that the content should be
 *   returned as. Make sure that returned content is run through check_plain()
 *   or other filtering functions when returning HTML.
 *
 * @return array
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
  $markup['#markup'] = _webform_references_get_node_title($plain_value, TRUE);
  return drupal_render($markup);
}

/**
 * Helper function to get title of node.
 *
 * @param int $nid
 *   Node Id.
 * @param bool $link
 *   FALSE for plain text and TRUE for linked text.
 *
 * @return string
 *   Title of the node (Linked if $link parameter is set to TRUE).
 */
function _webform_references_get_node_title($nid, $link = FALSE) {
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
 *
 * The output of this function will be displayed under the "Results" tab then
 * "Download".
 *
 * @param array $component
 *   A Webform component array.
 * @param array $export_options
 *   An array of options that may configure export of this field.
 *
 * @return array
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
 *
 * The output of this function will be displayed under the "Results" tab then
 * "Download".
 *
 * @param array $component
 *   A Webform component array.
 * @param array $export_options
 *   An array of options that may configure export of this field.
 * @param array $value
 *   An array of information containing the submission result, directly
 *   correlating to the webform_submitted_data database schema.
 *
 * @return string
 *   A string of item to be added to the CSV file.
 */
function _webform_csv_data_node_reference($component, $export_options, $value) {
  return !isset($value[0]) ? '' : _webform_references_get_node_title($value[0]);
}
