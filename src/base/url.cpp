/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
** 
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <vector>

#include "foreach.h"
#include "string.h"
#include "url.h"

Url::Url()
    : protocol(base::http::kHttp), port(0) {
}

Url::Url(const std::wstring& url)
    : protocol(base::http::kHttp), port(0) {
  Crack(url);
}

////////////////////////////////////////////////////////////////////////////////

Url& Url::operator=(const Url& url) {
  protocol = url.protocol;
  host = url.host;
  port = url.port;
  path = url.path;
  query = url.query;
  fragment = url.fragment;

  return *this;
}

void Url::operator=(const std::wstring& url) {
  Crack(url);
}

////////////////////////////////////////////////////////////////////////////////

void Url::Clear() {
  protocol = base::http::kHttp;
  host.clear();
  port = 0;
  path.clear();
  query.clear();
  fragment.clear();
}

std::wstring Url::Build() const {
  std::wstring url;

  switch (protocol) {
    case base::http::kHttp:
    default:
      url += L"http";
      break;
    case base::http::kHttps:
      url += L"https";
      break;
  }
  url += L"://";

  url += host;

  if (port > 0)
    url += L":" + ToWstr(port);

  url += path;

  if (!query.empty()) {
    std::wstring query_string;
    foreach_(it, query) {
      query_string += query_string.empty() ? L"?" : L"&";
      query_string += it->first + L"=" + EncodeUrl(it->second, false);
    }
    url += query_string;
  }

  if (!fragment.empty())
    url += L"#" + fragment;

  return url;
}

void Url::Crack(std::wstring url) {
  Clear();

  // Get protocol
  size_t i = url.find(L"://", 0);
  if (i != std::wstring::npos) {
    std::wstring scheme = url.substr(0, i);
    url = url.substr(i + 3);
    if (IsEqual(scheme, L"https"))
      protocol = base::http::kHttps;
  }

  // Get host and path
  i = url.find(L"/", 0);
  if (i == std::wstring::npos)
    i = url.length();
  host = url.substr(0, i);
  path = url.substr(i);

  // Get port number
  i = host.find(L":", 0);
  if (i != std::wstring::npos) {
    port = ToInt(host.substr(i + 1));
    host = host.substr(0, i);
  }

  // Get fragment
  i = path.find(L"#", 0);
  if (i != std::wstring::npos) {
    fragment = path.substr(i + 1);
    path = path.substr(0, i);
  }

  // Get query string
  i = path.find(L"?", 0);
  if (i != std::wstring::npos) {
    std::wstring query_string = path.substr(i + 1);
    path = path.substr(0, i);
    std::vector<std::wstring> parameters;
    Split(query_string, L"&", parameters);
    foreach_(it, parameters) {
      i = it->find(L"=", 0);
      if (i != std::wstring::npos) {
        std::wstring name = it->substr(0, i);
        std::wstring value = DecodeUrl(it->substr(i + 1));
        query.insert(std::make_pair(name, value));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

std::wstring DecodeUrl(const std::wstring& input) {
  std::string output;
  output.reserve(input.size());

  for (size_t i = 0; i < input.length(); i++) {
    if (input[i] == L'%' &&
        i + 2 < input.length() &&
        IsHex(input[i + 1]) && IsHex(input[i + 2])) {
      char c = 0;
      static const wchar_t* digits = L"0123456789ABCDEF";
      for (size_t j = 0; j < 16; j++) {
        if (input[i + 1] == digits[j])
          c += j << 4;
        if (input[i + 2] == digits[j])
          c += j;
      }
      output.append(1, c);
      i += 2;
    } else {
      output.append(1, static_cast<char>(input[i]));
    }
  }

  return StrToWstr(output);
}

std::wstring EncodeUrl(const std::wstring& input, bool encode_unreserved) {
  std::string str = WstrToStr(input);

  std::wstring output;
  output.reserve(input.size() * 2);

  for (size_t i = 0; i < str.length(); i++) {
    if ((str[i] >= '0' && str[i] <= '9') ||
        (str[i] >= 'A' && str[i] <= 'Z') ||
        (str[i] >= 'a' && str[i] <= 'z') ||
        (!encode_unreserved &&
         (str[i] == '-' || str[i] == '.' ||
          str[i] == '_' || str[i] == '~'))) {
      output.append(1, static_cast<wchar_t>(str[i]));
    } else {
      static const wchar_t* digits = L"0123456789ABCDEF";
      output.append(L"%");
      output.append(&digits[(str[i] >> 4) & 0x0F], 1);
      output.append(&digits[str[i] & 0x0F], 1);
    }
  }

  return output;
}