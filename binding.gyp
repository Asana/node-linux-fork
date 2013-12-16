{
  'conditions': [
    ['OS=="linux"', {
      "targets": [
        {
          "target_name": "fork",
          "sources": [ "src/fork.cc" ]
        }
      ]
    }],
    ['OS=="mac"', {
      "targets": [
        {
          "target_name": "fork",
          "sources": []
        }
      ]
    }]
  ]

}
