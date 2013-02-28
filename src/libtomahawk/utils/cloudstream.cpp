/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cloudstream.h"

#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>

#include <taglib/id3v2framefactory.h>
#include <taglib/mpegfile.h>

#include "utils/Logger.h"



namespace {
  static const int kTaglibPrefixCacheBytes = 64 * 1024;  // Should be enough.
  static const int kTaglibSuffixCacheBytes = 8 * 1024;
}

CloudStream::CloudStream(
    const QUrl& url, const QString& filename, const long length,
    const  QVariantMap& headers, QNetworkAccessManager* network)
    : url_(url),
      filename_(filename),
      encoded_filename_(filename_.toUtf8()),
      length_(length),
      headers_(headers),
      cursor_(0),
      network_(network),
      cache_(length),
      num_requests_(0) {
    tDebug( LOGINFO ) << "#### Cloudstream : CloudStream object created for " << filename_ << " : " << url_.toString();
}

TagLib::FileName CloudStream::name() const {
  return encoded_filename_.data();
}

bool CloudStream::CheckCache(int start, int end) {
  for (int i = start; i <= end; ++i) {
    if (!cache_.test(i)) {
      return false;
    }
  }
  return true;
}

void CloudStream::FillCache(int start, TagLib::ByteVector data) {
  for (int i = 0; i < data.size(); ++i) {
    cache_.set(start + i, data[i]);
  }
}

TagLib::ByteVector CloudStream::GetCached(int start, int end) {
  const uint size = end - start + 1;
  TagLib::ByteVector ret(size);
  for (int i = 0; i < size; ++i) {
    ret[i] = cache_.get(start + i);
  }
  return ret;
}

void CloudStream::Precache() {
  // For reading the tags of an MP3, TagLib tends to request:
  // 1. The first 1024 bytes
  // 2. Somewhere between the first 2KB and first 60KB
  // 3. The last KB or two.
  // 4. Somewhere in the first 64KB again
  //
  // OGG Vorbis may read the last 4KB.
  //
  // So, if we precache the first 64KB and the last 8KB we should be sorted :-)
  // Ideally, we would use bytes=0-655364,-8096 but Google Drive does not seem
  // to support multipart byte ranges yet so we have to make do with two
  // requests.
  tDebug( LOGINFO ) << "#### CloudStream : Precaching from :" << filename_;
  seek(0, TagLib::IOStream::Beginning);
  readBlock(kTaglibPrefixCacheBytes);
  seek(kTaglibSuffixCacheBytes, TagLib::IOStream::End);
  readBlock(kTaglibSuffixCacheBytes);
  clear();
  tDebug( LOGINFO ) << "#### CloudStream : Precaching end for :" << filename_;
}

TagLib::ByteVector CloudStream::readBlock(ulong length) {

  const uint start = cursor_;
  const uint end = qMin(cursor_ + length - 1, length_ - 1);

  //tDebug( LOGINFO ) << "#### CloudStream : reading block from " << start << " to " << end << " for " << url_.toString();

  if (end < start) {
    return TagLib::ByteVector();
  }

  if (CheckCache(start, end)) {
    TagLib::ByteVector cached = GetCached(start, end);
    cursor_ += cached.size();
    return cached;
  }

  QString authorizationHeader  = headers_["Authorization"].toString();
  QStringList authorizations = authorizationHeader.split(",");
  QStringList oneAuthList;
  QStringList newAuthorizationHeader;
  foreach(const QString& oneAuth, authorizations){
      if(auth.contains("oauth_nonce")){
          oneAuthList = oneAuth.split("=");
          QString oauthNonce = oneAuthList[1].replace('"',"");
          int newOautNonce = oauthNonce.toInt(0,16);
          newOautNonce++;

          oauthNonce = QString::number(newOautNonce,16);

          newAuthorizationHeader.append(authList[0]+"=\""+oauthNonce+"\"");
      }
      else {
          newAuthorizationHeader.append(oneAuth);
      }
  }

  headers_["Authorization"] = newAuthorizationHeader.join(", ");

  QNetworkRequest request = QNetworkRequest(url_);
  //setings of specials OAuth (1 or 2) headers

  foreach(const QString& headerName, headers_.keys()) {
      request.setRawHeader(headerName.toLocal8Bit(), headers_[headerName].toString().toLocal8Bit());

  }

  request.setRawHeader(
      "Range", QString("bytes=%1-%2").arg(start).arg(end).toUtf8());
  request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                       QNetworkRequest::AlwaysNetwork);
  // The Ubuntu One server applies the byte range to the gzipped data, rather
  // than the raw data so we must disable compression.
  if (url_.host() == "files.one.ubuntu.com") {
    request.setRawHeader("Accept-Encoding", "identity");
  }

  //tDebug() << request.rawHeader("Authorization");
  tDebug() << "######## CloudStream : HTTP request : ";
  foreach(const QByteArray& header, request.rawHeaderList()){
      tDebug() << "#### CloudStream : header request : " << header << " = " << request.rawHeader(header);
  }

  QNetworkReply* reply = network_->get(request);
  connect(reply, SIGNAL(sslErrors(QList<QSslError>)), SLOT(SSLErrors(QList<QSslError>)));
  ++num_requests_;

  QEventLoop loop;
  QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  loop.exec();
  reply->deleteLater();

  int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  tDebug() << "######## CloudStream : HTTP reply : ";
  tDebug( LOGINFO ) << "#### Cloudstream : HttpStatusCode : " << code;
  foreach (const QNetworkReply::RawHeaderPair& pair, reply->rawHeaderPairs()){
      tDebug( LOGINFO ) << "#### Cloudstream : header reply " << pair;
  }

  if (code >= 400) {
      tDebug( LOGINFO ) << "#### Cloudstream : Error " << code << " retrieving url to tag for " << filename_;
    return TagLib::ByteVector();
  }

  QByteArray data = reply->readAll();
  TagLib::ByteVector bytes(data.data(), data.size());
  cursor_ += data.size();

  FillCache(start, bytes);
  return bytes;
}

void CloudStream::writeBlock(const TagLib::ByteVector&) {
  tDebug( LOGINFO ) << "writeBlock not implemented";
}

void CloudStream::insert(const TagLib::ByteVector&, ulong, ulong) {
  tDebug( LOGINFO ) << "insert not implemented";
}

void CloudStream::removeBlock(ulong, ulong) {
  tDebug( LOGINFO ) << "removeBlock not implemented";
}

bool CloudStream::readOnly() const {
  tDebug( LOGINFO ) << "readOnly not implemented";
  return true;
}

bool CloudStream::isOpen() const {
  return true;
}

void CloudStream::seek(long offset, TagLib::IOStream::Position p) {
  switch (p) {
    case TagLib::IOStream::Beginning:
      cursor_ = offset;
      break;

    case TagLib::IOStream::Current:
      cursor_ = qMin(ulong(cursor_ + offset), length_);
      break;

    case TagLib::IOStream::End:
      // This should really not have qAbs(), but OGG reading needs it.
      cursor_ = qMax(0UL, length_ - qAbs(offset));
      break;
  }
}

void CloudStream::clear() {
  cursor_ = 0;
}

long CloudStream::tell() const {
  return cursor_;
}

long CloudStream::length() {
  return length_;
}

void CloudStream::truncate(long) {
  tDebug( LOGINFO ) << "not implemented";
}

void CloudStream::SSLErrors(const QList<QSslError>& errors) {
  foreach (const QSslError& error, errors) {
    tDebug( LOGINFO ) << "#### Cloudstream : Error for " << filename_ << " : ";
    tDebug( LOGINFO ) << error.error() << error.errorString();
    tDebug( LOGINFO ) << error.certificate();
  }
}
