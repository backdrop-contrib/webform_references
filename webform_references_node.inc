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
        '#element_validate' => array('webform_validate_node_reference'),
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
      );
      break;
  }
  return $form_item;
}

/**
 * Validation Callback for node reference field.
 */
function webform_validate_node_reference($element, $form_state) {
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
 */
function _webform_csv_data_node_reference($component, $export_options, $value) {
  return !isset($value[0]) ? '' : _webform_references_get_node_title($value[0]);
}
