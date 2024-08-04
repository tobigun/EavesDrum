import js from '@eslint/js'
import globals from 'globals'
import react from 'eslint-plugin-react'
import reactHooks from 'eslint-plugin-react-hooks';
import headers from 'eslint-plugin-headers';
import tseslint from 'typescript-eslint'

export default tseslint.config(
  {
    extends: [
      js.configs.recommended,
      ...tseslint.configs.recommended,
      ...tseslint.configs.stylistic,
      reactHooks.configs['recommended-latest']
    ],
    files: ['**/*.{ts,tsx}'],
    languageOptions: {
      ecmaVersion: 2022,
      globals: globals.browser,
    },
    settings: {
      react: {
        version: 'detect'
      }
    },
    plugins: {
      react,
      headers: headers
    },
    rules: {
      ...react.configs.recommended.rules,
      ...react.configs['jsx-runtime'].rules,
      "@typescript-eslint/no-explicit-any": "off",
      indent: ["error", 2],
      semi: ["error", "always"],
      "headers/header-format": [
        "error",
        {
          style: "line",
          trailingNewlines: 2,
          source: "string",
          content:
            "Copyright (c) 2025 Tobias Gunkel\n" +
            "SPDX-License-Identifier: GPL-3.0-or-later"
        }
      ]
    },
  },
)
